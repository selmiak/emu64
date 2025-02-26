//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: c64_keyboard_window.cpp               //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek       	//
//                                              //
// Letzte Änderung am 10.06.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "./c64_keyboard_window.h"
#include "./ui_c64_keyboard_window.h"

static const int y_min[7] = {12,60,107,153,153,177,199};
static const int y_max[7] = {59,106,152,198,174,198,247};
static const int keys_y[7] ={18,17,17,15,2,2,1};
static const int x_min[7][18] = {{18,67,114,161,208,255,303,350,397,444,492,539,585,633,680,727,797,834},
                                {18,90,138,184,232,279,326,374,420,469,515,561,609,656,704,797,834},
                                {9,55,103,150,197,244,291,339,387,433,480,527,575,622,669,797,834},
                                {9,55,127,174,221,268,316,363,410,457,505,552,600,797,834},
                                {669,718},
                                {669,718},
                                {137}};
static const int x_max[7][18] = {{66,113,160,207,254,302,349,396,443,491,538,584,632,679,726,774,833,868},
                                {89,137,183,231,278,325,373,419,468,514,560,608,655,703,774,833,868},
                                {54,102,149,196,243,290,338,386,432,479,526,574,621,668,765,833,868},
                                {54,126,173,220,267,315,362,409,456,504,551,599,668,833,868},
                                {717,765},
                                {717,765},
                                {561}};

/// TansTabelle von LeyLayout zu C64Matrix
static uint8_t vk_to_c64[7][18] = {{0x71,0x70,0x73,0x10,0x13,0x20,0x23,0x30,0x33,0x40,0x43,0x50,0x53,0x60,0x63,0x00,0x04,0x84},
                                         {0x72,0x76,0x11,0x16,0x21,0x26,0x31,0x36,0x41,0x46,0x51,0x56,0x61,0x66,0x80,0x05,0x85},
                                         {0x77,0x17,0x12,0x15,0x22,0x25,0x32,0x35,0x42,0x45,0x52,0x55,0x62,0x65,0x01,0x06,0x86},
                                         {0x75,0x17,0x14,0x27,0x24,0x37,0x34,0x47,0x44,0x57,0x54,0x67,0x64,0x03,0x83},
                                         {0x87,0x82},
                                         {0x07,0x02},
                                         {0x74}};
static bool vk_rast[7][18];

static uint8_t key_matrix_to_port_a_lm[8];
static uint8_t key_matrix_to_port_b_lm[8];
static uint8_t key_matrix_to_port_a_rm[8];
static uint8_t key_matrix_to_port_b_rm[8];

C64KeyboardWindow::C64KeyboardWindow(QWidget *parent, QSettings *ini, C64Class *c64) :
    QDialog(parent),
    ui(new Ui::C64KeyboardWindow)
{
    this->c64 = c64;
    this->ini = ini;

    ui->setupUi(this);

    for(int i=0; i<7; i++)
        for(int j=0; j<18; j++)
            vk_rast[i][j] = false;

    ws_x = width();
    ws_y = height();

    rec_key_press = false;
    rec_key_curr_x = 0xFF;
    rec_key_curr_y = 0xFF;

    key_matrix_to_port_a = nullptr;
    key_matrix_to_port_b = nullptr;

    recording = false;

    is_one_showed = false;
    current_y_key_old = 0xFF;
    current_y_key = 0xFF;
    current_x_key_old = 0xFF;
    current_x_key = 0xFF;

    setMouseTracking(true);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),this,SLOT(TimerEvent()));
    timer->start(200);

    ////////// Load from INI ///////////
    if(ini != nullptr)
    {
        ini->beginGroup("C64KeyboardWindow");
        if(ini->contains("Geometry")) restoreGeometry(ini->value("Geometry").toByteArray());
        //if(ini->value("Show",false).toBool()) show();
        ini->endGroup();
    }
    ////////////////////////////////////
}

C64KeyboardWindow::~C64KeyboardWindow(void)
{
    ////////// Save to INI ///////////
    if(ini != nullptr)
    {
        ini->beginGroup("C64KeyboardWindow");
        if(is_one_showed) ini->setValue("Geometry",saveGeometry());
        if(isHidden()) ini->setValue("Show",false);
        else ini->setValue("Show",true);
        ini->endGroup();
    }
    ////////////////////////////////////
    delete timer;
    delete ui;
}

void C64KeyboardWindow::RetranslateUi(void)
{
    ui->retranslateUi(this);
    this->update();
}

void C64KeyboardWindow::resizeEvent(QResizeEvent *event)
{
    scaling_x = static_cast<float_t>(event->size().width()) / 880.0f;
    scaling_y = static_cast<float_t>(event->size().height()) / 260.0f;
}

void C64KeyboardWindow::showEvent(QShowEvent*)
{
    is_one_showed = true;
}

void C64KeyboardWindow::mouseMoveEvent(QMouseEvent *event)
{
    static int xmin,xmax,ymin,ymax;

    QPoint mousePos = event->pos();

    int mx = mousePos.x();
    int my = mousePos.y();

    for(uint8_t i=0;i<7;i++)
    {
        ymin = int(y_min[i] * scaling_y);
        ymax = int(y_max[i] * scaling_y);

        if(my>=ymin && my<=ymax)
        {
            current_y_key = i;

            for(uint8_t j=0; j<keys_y[current_y_key]; j++)
            {
                xmin = int(x_min[current_y_key][j] * scaling_x);
                xmax = int(x_max[current_y_key][j] * scaling_x);

                if(mx>=xmin && mx<=xmax)
                {
                    current_x_key = j;
                    goto Lend;
                }
                else current_x_key = 0xFF;
            }
        }
        else current_y_key = 0xFF;
    }

Lend:

    if((current_y_key != current_y_key_old) || (current_x_key != current_x_key_old))
    {
        current_y_key_old = current_y_key;
        current_x_key_old = current_x_key;

        if((current_y_key != 0xFF) && (current_x_key != 0xFF))
        {
            update();
            if ((event->buttons() & Qt::LeftButton) == Qt::LeftButton)
            {                
                if(!recording)
                {
                    for(uint8_t i=0; i<8; i++)
                    {
                        key_matrix_to_port_a_lm[i]=0;
                        key_matrix_to_port_b_lm[i]=0;
                    }
                    uint8_t C64Key = vk_to_c64[current_y_key][current_x_key];

                    vk_rast[current_y_key][current_x_key] = false;
                    key_matrix_to_port_b_rm[(C64Key>>4)&0xF] &= ~(1<<(C64Key&0xF));
                    key_matrix_to_port_b_lm[(C64Key>>4)&0xF] |= 1<<(C64Key&0xF);

                    key_matrix_to_port_a_rm[C64Key&0xF] &= ~(1<<((C64Key>>4)&0xF));
                    key_matrix_to_port_a_lm[C64Key&0xF] |= 1<<((C64Key>>4)&0xF);

                    for(uint8_t i=0; i<8; i++)
                    {
                        if (key_matrix_to_port_a != nullptr) key_matrix_to_port_a[i] = key_matrix_to_port_a_lm[i] | key_matrix_to_port_a_rm[i];
                        if (key_matrix_to_port_b != nullptr) key_matrix_to_port_b[i] = key_matrix_to_port_b_lm[i] | key_matrix_to_port_b_rm[i];
                    }
                }
            }
        }
        else
        {
            update();
        }
    }
}

void C64KeyboardWindow::mousePressEvent(QMouseEvent *event)
{
   //QPoint mouse_offset = event->pos();

    if(!recording)
    {
        if((event->buttons() & Qt::LeftButton) == Qt::LeftButton)
        {
            if(current_x_key == 0xFF || current_y_key == 0xFF) return;

            uint8_t C64Key = vk_to_c64[current_y_key][current_x_key];

            //// RESTORE TASTE ---> NMI C64 ///
            if((C64Key & 0x0100) == 0x0100)
            {
                    //C64::SetRestore();
                    return;
            }
            ///////////////////////////////////

            vk_rast[current_y_key][current_x_key] = false;

            if(C64Key < 128)
            {
                key_matrix_to_port_b_rm[(C64Key>>4)&0xF] &= ~(1<<(C64Key&0xF));
                key_matrix_to_port_b_lm[(C64Key>>4)&0xF] |= 1<<(C64Key&0xF);

                key_matrix_to_port_a_rm[C64Key&0xF] &= ~(1<<((C64Key>>4)&0xF));
                key_matrix_to_port_a_lm[C64Key&0xF] |= 1<<((C64Key>>4)&0xF);
            }
            else
            {
                C64Key &= ~128;
                key_matrix_to_port_b_rm[(C64Key>>4)&0xF] &= ~(1<<(C64Key&0xF));
                key_matrix_to_port_b_lm[(C64Key>>4)&0xF] |= 1<<(C64Key&0xF);

                key_matrix_to_port_a_rm[C64Key&0xF] &= ~(1<<((C64Key>>4)&0xF));
                key_matrix_to_port_a_lm[C64Key&0xF] |= 1<<((C64Key>>4)&0xF);

                C64Key = 23;    // linke Shiftaste (100 = rechte)
                key_matrix_to_port_b_rm[(C64Key>>4)&0xF] &= ~(1<<(C64Key&0xF));
                key_matrix_to_port_b_lm[(C64Key>>4)&0xF] |= 1<<(C64Key&0xF);

                key_matrix_to_port_a_rm[C64Key&0xF] &= ~(1<<((C64Key>>4)&0xF));
                key_matrix_to_port_a_lm[C64Key&0xF] |= 1<<((C64Key>>4)&0xF);
            }
        }

        if((event->buttons() & Qt::RightButton) == Qt::RightButton)
        {
            if(current_x_key == 0xFF || current_y_key == 0xFF) return;

            uint8_t C64Key = vk_to_c64[current_y_key][current_x_key];

            if(vk_rast[current_y_key][current_x_key])
            {
                vk_rast[current_y_key][current_x_key] = false;
                key_matrix_to_port_b_rm[(C64Key>>4)&0xF] &= ~(1<<(C64Key&0xF));
                key_matrix_to_port_a_rm[C64Key&0xF] &= ~(1<<((C64Key>>4)&0xF));
            }
            else
            {
                vk_rast[current_y_key][current_x_key] = true;
                key_matrix_to_port_b_rm[(C64Key>>4)&0xF] |= 1<<(C64Key&0xF);
                key_matrix_to_port_a_rm[C64Key&0xF] |= 1<<((C64Key>>4)&0xF);
            }
        }

        for(int i=0;i<8;i++)
        {
            key_matrix_to_port_a[i] = key_matrix_to_port_a_lm[i] | key_matrix_to_port_a_rm[i];
            key_matrix_to_port_b[i] = key_matrix_to_port_b_lm[i] | key_matrix_to_port_b_rm[i];
        }
    }
    else
    {
        if((event->buttons() & Qt::LeftButton) == Qt::LeftButton)
        {
            if(current_x_key == 0xFF || current_y_key == 0xFF) return;

            rec_key_press = true;
            blink_flip = true;
            rec_time_out = 24;

            // Restore Taste
            /*
            if((AKT_Y_KEY == 1) && (AKT_X_KEY == 14))
            {
                if(c64 != nullptr) c64->StopRecKeyMap();
                RecKeyPress = false;
                blink_flip = false;
                return;
            }
            */

            // Shift Lock Taste
            if((current_y_key == 2) && (current_x_key == 1))
            {
                if(c64 != nullptr) c64->StopRecKeyMap();
                rec_key_press = false;
                blink_flip = false;
                return;
            }

            rec_key_curr_x = current_x_key;
            rec_key_curr_y = current_y_key;
            update();

            uint8_t matrix_code = vk_to_c64[rec_key_curr_y][rec_key_curr_x];

            c64->SetFocusToC64Window();
            c64->StartRecKeyMap(matrix_code);
            timer->start(200);
        }
    }
}

void C64KeyboardWindow::mouseReleaseEvent(QMouseEvent*)
{
    if(!recording)
    {
        for(int i=0;i<8;i++)
        {
            key_matrix_to_port_b_lm[i]=0;
            key_matrix_to_port_b[i] = key_matrix_to_port_b_lm[i] | key_matrix_to_port_b_rm[i];
            key_matrix_to_port_a[i] = key_matrix_to_port_a_lm[i] | key_matrix_to_port_a_rm[i];
        }
    }
}

void C64KeyboardWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QPen pen1(QBrush(QColor(255,255,0)),3);
    QPen pen2(QBrush(QColor(255,0,0)),3);
    QPen pen3(QBrush(QColor(0,0,255)),3);

    if((current_y_key != 0xFF) && (current_x_key != 0xFF))
    {
        painter.setPen(pen3);
        painter.drawRect(static_cast<int>(x_min[current_y_key][current_x_key] * scaling_x),
                         static_cast<int>(y_min[current_y_key] * scaling_y),
                         static_cast<int>((x_max[current_y_key][current_x_key]* scaling_x) - (x_min[current_y_key][current_x_key] * scaling_x)),
                         static_cast<int>((y_max[current_y_key] * scaling_y) - (y_min[current_y_key] * scaling_y)));
    }

    for(int y=0;y<7;y++)
    {
        for(int x=0;x<18;x++)
        {
            if(vk_rast[y][x])
            {
                painter.setPen(pen1);
                painter.drawRect(static_cast<int>(x_min[y][x] * scaling_x),
                                 static_cast<int>(y_min[y] * scaling_y),
                                 static_cast<int>((x_max[y][x]* scaling_x) - (x_min[y][x] * scaling_x)),
                                 static_cast<int>((y_max[y] * scaling_y) - (y_min[y] * scaling_y)));
            }
        }
    }
    if(recording)
    {
        painter.setPen(pen2);
        painter.drawRect(0,0,this->width(),this->height());
        if(rec_key_press)
        {
            if(blink_flip)
            {
                painter.setPen(pen2);
                painter.drawRect(static_cast<int>(x_min[rec_key_curr_y][rec_key_curr_x] * scaling_x),
                                 static_cast<int>(y_min[rec_key_curr_y] * scaling_y),
                                 static_cast<int>((x_max[rec_key_curr_y][rec_key_curr_x]* scaling_x) - (x_min[rec_key_curr_y][rec_key_curr_x] * scaling_x)),
                                 static_cast<int>((y_max[rec_key_curr_y] * scaling_y) - (y_min[rec_key_curr_y] * scaling_y)));
            }
        }
    }
    painter.end();
}

void C64KeyboardWindow::TimerEvent()
{
    rec_time_out--;
    if(rec_time_out == 0 || c64->GetRecKeyMapStatus() == false)
    {
        timer->stop();
        c64->StopRecKeyMap();
        rec_key_press = false;
        blink_flip = false;
    }
    else
    {
        if(blink_flip)
        {
                blink_flip = false;
        }
        else blink_flip = true;
    }
    update();
}
