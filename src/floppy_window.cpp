//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: floppy_window.cpp                     //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 31.08.2019                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "./floppy_window.h"
#include "./ui_floppy_window.h"

#define MAX_D64_FILES 128

FloppyWindow::FloppyWindow(QWidget *parent, QSettings *_ini, C64Class *c64, QString tmp_path) :
    QDialog(parent),
    ui(new Ui::FloppyWindow)
{
    ini = _ini;
    ui->setupUi(this);

    isOneShowed = false;

    this->c64 = c64;
    this->tmp_path = tmp_path;

    ui->FileBrowser->SetTempDir(tmp_path);

    FileTypes = QStringList() << "DEL" << "SEQ" << "PRG" << "USR" << "REL" << "CBM" << "E00" << "E?C";

    green_led_small = new QIcon(":/grafik/green_led_7x7.png");
    yellow_led_small = new QIcon(":/grafik/yellow_led_7x7.png");
    red_led_small = new QIcon(":/grafik/red_led_7x7.png");

    green_led_big = new QIcon(":/grafik/green_led_on.png");
    yellow_led_big = new QIcon(":/grafik/yellow_led_on.png");
    red_led_big = new QIcon(":/grafik/red_led_on.png");

    QFontDatabase::addApplicationFont(":/fonts/C64_Pro-STYLE.ttf");
    c64_font1 = new QFont("C64 Pro");
    c64_font1->setPixelSize(8);
    c64_font1->setStyleStrategy(QFont::PreferAntialias);
    c64_font1->setBold(false);
    c64_font1->setKerning(true);

    c64_font2 = new QFont("C64 Pro");
    c64_font2->setPixelSize(16);
    c64_font2->setStyleStrategy(QFont::PreferAntialias);
    c64_font2->setBold(false);
    c64_font2->setKerning(true);

    c64_font = c64_font1;

    ui->DiskName->setFont(*c64_font2);

    // Spalten für D64 Datei-Anzeige hinzufügen
    ui->D64FileTable->setColumnCount(7);

    connect(ui->FileBrowser,SIGNAL(SelectFile(QString)),this,SLOT(OnSelectFile(QString)));
    connect(ui->FileBrowser,SIGNAL(WriteProtectedChanged(bool)),this,SLOT(OnWriteProtectedChanged(bool)));
    ui->FileBrowser->SetFileFilter(QStringList()<<"*.d64"<<"*.g64");
    ui->FileBrowser->EnableWriteProtectCheck(true);

    // Kontextmenü erstellen
    CompatibleMMCFileName = false;
    ui->D64FileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->D64FileTable, SIGNAL(customContextMenuRequested(QPoint)),SLOT(OnCustomMenuRequested(QPoint)));
}

FloppyWindow::~FloppyWindow()
{
    ////////// Save to INI ///////////
    if(ini != nullptr)
    {
        ini->beginGroup("FloppyWindow");
        if(isOneShowed)
        {
            ini->setValue("Geometry",saveGeometry());
            ini->setValue("ViewSplatFile",ui->ViewSplatFiles->isChecked());
            ini->setValue("D64ViewBigSize",ui->D64FileTableBigSize->isChecked());
            ini->setValue("PrgExportMMCCompatible",CompatibleMMCFileName);
        }

        if(isHidden()) ini->setValue("Show",false);
        else ini->setValue("Show",true);
        ini->endGroup();

        char group_name[32];
        for(int i=0; i<MAX_FLOPPY_NUM; i++)
        {
            sprintf(group_name,"Floppy1541_%2.2X",i+8);
            ini->beginGroup(group_name);
            ini->setValue("AktDir",AktDir[i]);
            ini->setValue("AktFile",AktFile[i]);
            ini->endGroup();
        }
    }
    ////////////////////////////////////

    delete green_led_small;
    delete yellow_led_small;
    delete red_led_small;
    delete green_led_big;
    delete yellow_led_big;
    delete red_led_big;

    delete c64_font1;
    delete c64_font2;

    delete ui;
}

void FloppyWindow::showEvent(QShowEvent*)
{
    isOneShowed = true;
}

void FloppyWindow::LoadIni()
{
    ////////// Load from INI ///////////
    if(ini != nullptr)
    {
        ini->beginGroup("FloppyWindow");
        if(ini->contains("Geometry")) restoreGeometry(ini->value("Geometry").toByteArray());
        ui->ViewSplatFiles->setChecked(ini->value("ViewSplatFile",false).toBool());
        ui->D64FileTableBigSize->setChecked(ini->value("D64ViewBigSize",false).toBool());
        CompatibleMMCFileName = ini->value("PrgExportMMCCompatible", true).toBool();
        ini->endGroup();

        // ViewD64BigSize setzen
        SetD64BigSize(ui->D64FileTableBigSize->isChecked());

        char group_name[32];
        for(int i=0; i<MAX_FLOPPY_NUM; i++)
        {
            sprintf(group_name,"Floppy1541_%2.2X",i+8);
            ini->beginGroup(group_name);

            AktDir[i] = ini->value("AktDir","").toString();
            AktFile[i] = ini->value("AktFile","").toString();
            AktFileName[i] = AktDir[i] + "/" + AktFile[i];
            d64[i].LoadD64(AktFileName[i].toUtf8());

            // Write Protect für alle Floppys setzen
            c64->floppy[i]->SetWriteProtect(ui->FileBrowser->isFileWriteProtect(AktFileName[i]));

            ini->endGroup();
        }

        ui->FileBrowser->SetAktDir(AktDir[0]);
        ui->FileBrowser->SetAktFile(AktDir[0],AktFile[0]);

        RefreshD64FileList();

        // Write Protect nochmal für alle Floppy 0 setzen
        c64->floppy[0]->SetWriteProtect(ui->FileBrowser->isFileWriteProtect(AktDir[0] + "/" + AktFile[0]));
    }
    ////////////////////////////////////
}

bool FloppyWindow::SetDiskImage(uint8_t floppynr, QString filename)
{
    if(floppynr >= MAX_FLOPPY_NUM) return false;

    QFileInfo fi = QFileInfo(filename);

    AktDir[floppynr] = fi.absolutePath();
    AktFile[floppynr] = fi.fileName();
    AktFileName[floppynr] = AktDir[floppynr] + "/" + AktFile[floppynr];
    d64[floppynr].LoadD64(AktFileName[floppynr].toUtf8());

    c64->floppy[floppynr]->SetWriteProtect(ui->FileBrowser->isFileWriteProtect(AktFileName[floppynr]));

    ui->FloppySelect->setCurrentIndex(floppynr);
    ui->FileBrowser->SetAktDir(AktDir[floppynr]);
    ui->FileBrowser->SetAktFile(AktDir[floppynr],AktFile[floppynr]);

    RefreshD64FileList();

    return true;
}

void FloppyWindow::RetranslateUi()
{
    ui->retranslateUi(this);
    ui->FileBrowser->RetranslateUi();
    this->update();
}

void FloppyWindow::OnSelectFile(QString filename)
{
    int FloppyNr = ui->FloppySelect->currentIndex();

    if(FloppyNr < MAX_FLOPPY_NUM)
    {
        AktFileName[FloppyNr] = filename;
        AktDir[FloppyNr] = ui->FileBrowser->GetAktDir();
        AktFile[FloppyNr] = ui->FileBrowser->GetAktFile();
        if("D64" == AktFileName[FloppyNr].right(3).toUpper())
        {
            d64[FloppyNr].LoadD64(AktFileName[FloppyNr].toUtf8());
            RefreshD64FileList();
            emit ChangeFloppyImage(FloppyNr);
        }

        if("G64" == AktFileName[FloppyNr].right(3).toUpper())
        {
            //d64[FloppyNr].LoadD64(AktFileName[FloppyNr].toUtf8());
            //RefreshD64FileList();


            // D64 File-Anzeige löschen
            ui->D64FileTable->clear();

            // D64 Name ermitteln und anzeigen
            ui->DiskName->setText("G64 IMAGE");

            emit ChangeFloppyImage(FloppyNr);
        }
    }
}

void FloppyWindow::OnChangeFloppyNummer(int floppynr)
{
    if(this->isHidden())
        this->setHidden(false);
    ui->FloppySelect->setCurrentIndex(floppynr);
}

void FloppyWindow::OnRemoveImage(int floppynr)
{
    AktFile[floppynr] = "";
    AktFileName[floppynr] = AktDir[floppynr] + "/" + AktFile[floppynr];

    if(floppynr == ui->FloppySelect->currentIndex())
    {
        ui->FileBrowser->SetAktDir(AktDir[floppynr]);
        ui->FileBrowser->SetAktFile(AktDir[floppynr],AktFile[floppynr]);
    }
}

void FloppyWindow::on_FloppySelect_currentIndexChanged(int index)
{
    if(index < 0) return;

    if(isOneShowed)
    {
        ui->FileBrowser->SetAktDir(AktDir[index]);
        ui->FileBrowser->SetAktFile(AktDir[index],AktFile[index]);
        RefreshD64FileList();
    }
}

void FloppyWindow::OnCustomMenuRequested(QPoint pos)
{
    QMenu *menu=new QMenu(this);
    menu->addAction(new QAction(tr("Laden und Starten mit Reset ohne Kernal"), this));
    menu->addAction(new QAction(tr("Laden und Starten mit Reset"), this));
    menu->addAction(new QAction(tr("Laden und Starten"), this));
    menu->addAction(new QAction(tr("Laden ohne Kernal"), this));
    menu->addAction(new QAction(tr("Laden"), this));

    QMenu *menu_export = new QMenu(this);
    menu_export->setTitle("Export");
    menu_export->addAction(new QAction(tr("Datei exportieren"), this));
    menu_export->addAction(new QAction(tr("Dateiname MMC/SD2IEC kompatibel"), this));
    menu_export->actions().at(1)->setCheckable(true);
    menu_export->actions().at(1)->setChecked(CompatibleMMCFileName);

    menu->addMenu(menu_export);

    // Oberster Eintrag hervorheben
    QFont font = menu->actions().at(0)->font();
    font.setBold(true);
    menu->actions().at(0)->setFont(font);

    menu->popup(ui->D64FileTable->viewport()->mapToGlobal(pos));
    connect(menu->actions().at(0),SIGNAL(triggered(bool)),this,SLOT(OnD64FileStart0(bool)));
    connect(menu->actions().at(1),SIGNAL(triggered(bool)),this,SLOT(OnD64FileStart1(bool)));
    connect(menu->actions().at(2),SIGNAL(triggered(bool)),this,SLOT(OnD64FileStart2(bool)));
    connect(menu->actions().at(3),SIGNAL(triggered(bool)),this,SLOT(OnD64FileStart3(bool)));
    connect(menu->actions().at(4),SIGNAL(triggered(bool)),this,SLOT(OnD64FileStart4(bool)));
    connect(menu_export->actions().at(0),SIGNAL(triggered(bool)),this,SLOT(OnPRGExport(bool)));
    connect(menu_export->actions().at(1),SIGNAL(triggered(bool)),this,SLOT(OnPRGNameMMCKompatibel(bool)));
}

void FloppyWindow::OnD64FileStart0(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    uint8_t floppy_nr = static_cast<uint8_t>(ui->FloppySelect->currentIndex());
    QString FileName = tmp_path + "/tmp.prg";

    if(d64[floppy_nr].ExportPrg(file_index,FileName.toUtf8().data()))
    {
        c64->LoadAutoRun(floppy_nr,FileName.toUtf8().data());
    }
}

void FloppyWindow::OnD64FileStart1(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    uint8_t floppy_nr = static_cast<uint8_t>(ui->FloppySelect->currentIndex());
    c64->LoadPRGFromD64(floppy_nr,d64[floppy_nr].d64_files[file_index].Name,0);
}

void FloppyWindow::OnD64FileStart2(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    uint8_t floppy_nr = static_cast<uint8_t>(ui->FloppySelect->currentIndex());
    c64->LoadPRGFromD64(floppy_nr,d64[floppy_nr].d64_files[file_index].Name,1);
}

void FloppyWindow::OnD64FileStart3(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    uint8_t floppy_nr = static_cast<uint8_t>(ui->FloppySelect->currentIndex());
    QString FileName = tmp_path + "/tmp.prg";

    if(d64[floppy_nr].ExportPrg(file_index,FileName.toUtf8().data()))
    {
        c64->LoadPRG(FileName.toUtf8().data(),nullptr);
    }
}

void FloppyWindow::OnD64FileStart4(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    uint8_t floppy_nr = static_cast<uint8_t>(ui->FloppySelect->currentIndex());
    c64->LoadPRGFromD64(floppy_nr,d64[floppy_nr].d64_files[file_index].Name,2);
}

void FloppyWindow::OnPRGExport(bool)
{
    int file_index = ui->D64FileTable->currentIndex().row();
    int floppy_nr = ui->FloppySelect->currentIndex();

    // Alle nichterlaubten Zeichen in einem Dateinamen entfernen.
    char c64_filename[17];
    for (int i=0;i<17;i++)
    {
       c64_filename[i] = d64[floppy_nr].d64_files[file_index].Name[i];
       if(c64_filename[i]=='/') c64_filename[i]=' ';
       if(c64_filename[i]=='\\') c64_filename[i]=' ';
       if(c64_filename[i]==':') c64_filename[i]=' ';
       if(c64_filename[i]=='<') c64_filename[i]=' ';
       if(c64_filename[i]=='>') c64_filename[i]=' ';
       if(c64_filename[i]=='.') c64_filename[i]=' ';
    }
    c64_filename[16] = 0;

    QString filename = QString(c64_filename);
    QString fileext = FileTypes[d64[floppy_nr].d64_files[file_index].Typ & 7];

    if(CompatibleMMCFileName)
    {
        filename = filename.toLower();
        fileext = fileext.toLower();
    }

    if(c64 == nullptr) return;

    QStringList filters;
    filters << tr("C64 Programmdatei (*.prg *.seq *.usr *.rel *.cbm *.e00 *.del")
            << tr("Alle Dateien (*.*)");

    if(!CustomSaveFileDialog::GetSaveFileName(this, tr("C64 Datei Exportieren"), filters, &filename, &fileext))
        return;

    if(filename != "")
    {
        if(!d64[floppy_nr].ExportPrg(file_index,filename.toUtf8().data()))
        {
            QMessageBox::critical(this,"C64 Datei Export","Fehler beim exportieren der C64 Datei.\nDie Datei konnte nicht exortiert.");
        }
    }
}

void FloppyWindow::OnPRGNameMMCKompatibel(bool)
{
    if(CompatibleMMCFileName)
        CompatibleMMCFileName = false;
    else CompatibleMMCFileName = true;
}

void FloppyWindow::OnWriteProtectedChanged(bool wp_satus)
{
    c64->SetFloppyWriteProtect(static_cast<uint8_t>(ui->FloppySelect->currentIndex()), wp_satus);
}

QString FloppyWindow::GetAktFilename(int floppynr)
{
    return AktFileName[floppynr];
}

QString FloppyWindow::GetAktD64Name(int floppynr)
{
    return ConvC64Name(d64[floppynr].d64_name, true);
}

void FloppyWindow::RefreshD64FileList()
{
    // D64 File-Anzeige löschen
    ui->D64FileTable->clear();

    // Aktuell ausgewählte Floppy
    int floppy = ui->FloppySelect->currentIndex();
    // Anzahl der Files
    int d64_files = d64[floppy].file_count;

    // D64 Name ermitteln und anzeigen
    ui->DiskName->setText(ConvC64Name(d64[floppy].d64_name, true));

    // Headerlabels setzen
    ui->D64FileTable->setHorizontalHeaderLabels(QStringList() << "" << "DATEI" << "SP" << "SK" << "ADR" << "SIZE" << "TYP");

    // Anzahl der Zeilen setzen
    ui->D64FileTable->setRowCount(d64_files);
    for(int i=0; i<d64_files;i++)
    {
        // Icon in Abhängigkeit der Startbarkeit setzen
        QTableWidgetItem *icon_item = new QTableWidgetItem;
        if((d64[floppy].d64_files[i].Adresse==0x0801) && ((d64[floppy].d64_files[i].Typ & 7)==(2))) icon_item->setIcon(*green_led);
        else
        {
            if((d64[floppy].d64_files[i].Adresse<0x0801) && ((d64[floppy].d64_files[i].Typ & 7)==(2))) icon_item->setIcon(*yellow_led);
            else icon_item->setIcon(*red_led);
        }
        ui->D64FileTable->setItem(i,0,icon_item);

        ui->D64FileTable->setCellWidget(i,1,new QLabel(ConvC64Name(d64[floppy].d64_files[i].Name)));

        ui->D64FileTable->cellWidget(i,1)->setFont(*c64_font);
        QLabel* label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,1));
        label->setAlignment(Qt::AlignTop);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Spur setzen
        ui->D64FileTable->setCellWidget(i,2,new QLabel(QVariant(d64[floppy].d64_files[i].Track).toString() + " "));
        ui->D64FileTable->cellWidget(i,2)->setFont(*c64_font);
        label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,2));
        label->setAlignment(Qt::AlignTop | Qt::AlignRight);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Sektor setzen
        ui->D64FileTable->setCellWidget(i,3,new QLabel(QVariant(d64[floppy].d64_files[i].Sektor).toString() + " "));
        ui->D64FileTable->cellWidget(i,3)->setFont(*c64_font);
        label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,3));
        label->setAlignment(Qt::AlignTop | Qt::AlignRight);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Adresse setzen
        QString str;
        str.sprintf(" $%04X",d64[floppy].d64_files[i].Adresse);
        ui->D64FileTable->setCellWidget(i,4,new QLabel(str));
        ui->D64FileTable->cellWidget(i,4)->setFont(*c64_font);
        label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,4));
        label->setAlignment(Qt::AlignTop);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Size setzen
        ui->D64FileTable->setCellWidget(i,5,new QLabel(QVariant(d64[floppy].d64_files[i].Laenge).toString() + " "));
        ui->D64FileTable->cellWidget(i,5)->setFont(*c64_font);
        label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,5));
        label->setAlignment(Qt::AlignTop | Qt::AlignRight);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Typ setzen
        QString strTyp;
        int FileTyp = d64[floppy].d64_files[i].Typ;

        if(FileTyp & 0x40) strTyp = FileTypes[FileTyp & 0x07] + "<";
        else strTyp = FileTypes[FileTyp & 0x07];
        if(FileTyp & 0x80) strTyp = " " + strTyp;
        else strTyp = "*" + strTyp;

        ui->D64FileTable->setCellWidget(i,6,new QLabel(strTyp));
        ui->D64FileTable->cellWidget(i,6)->setFont(*c64_font);
        label = static_cast<QLabel*>(ui->D64FileTable->cellWidget(i,6));
        label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        label->setStyleSheet("color: rgb(100, 100, 255);");

        // Höhe der Zeile setzen
        if(ui->D64FileTableBigSize->isChecked())
        {
            ui->D64FileTable->setRowHeight(i,16);
            ui->D64FileTable->cellWidget(i,1)->setFixedHeight(16);
            ui->D64FileTable->cellWidget(i,1)->setMaximumHeight(16);
            ui->D64FileTable->cellWidget(i,1)->setMinimumHeight(16);
        }
        else
        {
            ui->D64FileTable->setRowHeight(i,8);
            ui->D64FileTable->cellWidget(i,1)->setFixedHeight(8);
            ui->D64FileTable->cellWidget(i,1)->setMaximumHeight(8);
            ui->D64FileTable->cellWidget(i,1)->setMinimumHeight(8);
        }

        if(!(FileTyp & 0x80) && !ui->ViewSplatFiles->isChecked())
        {
            ui->D64FileTable->setRowHeight(i,0);
        }
    }
}

void FloppyWindow::on_ViewSplatFiles_clicked()
{
    RefreshD64FileList();
}

void FloppyWindow::on_D64FileTable_cellDoubleClicked(int , int )
{
    OnD64FileStart0(0);
}

void FloppyWindow::SetD64BigSize(bool enable)
{
    if(enable)
    {
        c64_font = c64_font2;

        green_led = green_led_big;
        yellow_led = yellow_led_big;
        red_led = red_led_big;

        ui->D64FileTable->setMinimumWidth(395 * 2);
        ui->D64FileTable->setMaximumWidth(395 * 2);

        ui->D64FileTable->setColumnWidth(0,20 * 2);
        ui->D64FileTable->setColumnWidth(1,145 * 2);
        ui->D64FileTable->setColumnWidth(2,30 * 2);
        ui->D64FileTable->setColumnWidth(3,30 * 2);
        ui->D64FileTable->setColumnWidth(4,55 * 2);
        ui->D64FileTable->setColumnWidth(5,45 * 2);
        ui->D64FileTable->setColumnWidth(6,45 * 2);
    }
    else
    {
        c64_font = c64_font1;

        green_led = green_led_small;
        yellow_led = yellow_led_small;
        red_led = red_led_small;

        ui->D64FileTable->setMinimumWidth(395);
        ui->D64FileTable->setMaximumWidth(395);

        ui->D64FileTable->setColumnWidth(0,20);
        ui->D64FileTable->setColumnWidth(1,145);
        ui->D64FileTable->setColumnWidth(2,30);
        ui->D64FileTable->setColumnWidth(3,30);
        ui->D64FileTable->setColumnWidth(4,55);
        ui->D64FileTable->setColumnWidth(5,45);
        ui->D64FileTable->setColumnWidth(6,45);
    }
}

QString FloppyWindow::ConvC64Name(const char *name, bool invers)
{
    size_t size = strlen(name);
    char *new_name = new char[size+1];
    strcpy(new_name, name);

    QString name_str = "";
    for(size_t i=0; i<size; i++)
    {
        if(static_cast<uint8_t>(new_name[i]) != 0)
        {
            if(static_cast<uint8_t>(new_name[i]) == 32 || (static_cast<uint8_t>(new_name[i]) == 160))
            {
                if(!invers)
                    name_str += ' ';
                else
                    name_str += QChar(' ' + 0xe200);
            }
            else
            {
                if((static_cast<uint8_t>(new_name[i]) > 127) && (static_cast<uint8_t>(new_name[i]) < 160))
                {
                    if(!invers)
                        name_str += QChar((static_cast<uint8_t>(new_name[i])) + 0xee40);
                    else
                        name_str += QChar((static_cast<uint8_t>(new_name[i])) + 0xee40);
                }
                else
                {
                    if(!invers)
                    {
                        name_str += QChar((static_cast<uint8_t>(new_name[i])) + 0xe000);
                    }
                    else
                        name_str += QChar((static_cast<uint8_t>(new_name[i])) + 0xe200);
                }
            }
        }
    }
    delete [] new_name;

    return name_str;
}

void FloppyWindow::on_D64FileTableBigSize_clicked(bool checked)
{
    SetD64BigSize(checked);
    RefreshD64FileList();
}

void FloppyWindow::on_CreateNewD64_clicked()
{
    FloppyNewD64Window *floppy_new_d64_window = new FloppyNewD64Window(this);

    if(floppy_new_d64_window->exec())
    {
        QString filename = floppy_new_d64_window->GetFilename();
        QString diskname = floppy_new_d64_window->GetDiskname();
        QString diskid   = floppy_new_d64_window->GetDiskID();
        QString fullpath;

        if(filename.right(4).toUpper() != ".D64")
            filename += ".d64";

        fullpath = ui->FileBrowser->GetAktDir() + "/" + filename;

        QFile file(fullpath);
        if(!file.exists())
        {
            D64Class d64;
            if(!d64.CreateDiskImage(fullpath.toUtf8().data(),diskname.toUtf8().data(),diskid.toUtf8().data()))
            {
                QMessageBox::critical(this,tr("Fehler!"),tr("Es konnte kein neues Diskimage erstellt werden."));
            }
            else
            {
                ui->FileBrowser->SetAktDir(ui->FileBrowser->GetAktDir());
                ui->FileBrowser->SetAktFile(ui->FileBrowser->GetAktDir(),filename);
                RefreshD64FileList();
            }
        }
        else
        {
            if(QMessageBox::Yes == QMessageBox::question(this,tr("Achtung!"),tr("Eine Datei mit diesen Namen existiert schon!\nSoll diese überschrieben werden?"),QMessageBox::Yes | QMessageBox::No))
            {
                D64Class d64;
                if(!d64.CreateDiskImage(fullpath.toUtf8().data(),diskname.toUtf8().data(),diskid.toUtf8().data()))
                {
                    QMessageBox::critical(this,tr("Fehler!"),tr("Es konnte kein neues Diskimage erstellt werden."));
                }
                else
                {
                    ui->FileBrowser->SetAktDir(ui->FileBrowser->GetAktDir());
                    ui->FileBrowser->SetAktFile(ui->FileBrowser->GetAktDir(),filename);
                    RefreshD64FileList();
                }
            }
        }
    }

    delete floppy_new_d64_window;
}
