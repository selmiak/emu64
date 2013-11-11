#include <jni.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <android/log.h>


#define  LOG_TAG    "NATIVE"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "TAG", __VA_ARGS__);

#include "asset_texture_class.h"
#include "text_box_class.h"
#include "dsp.h"

/// Globale Variablen
JavaVM *vm;
AssetTextureClass *texture = NULL;
TextBoxClass *textBox01 = NULL;

char* text_buffer;
unsigned char start_wert = 0;

#ifdef __cplusplus
extern "C"
{
#endif

unsigned short sample = 0;

void SoundCallback(short *buffer, int num_samples)
{
	printf("%d",num_samples);

	for(int i=0;i<(num_samples/2);i++)
	{
		buffer[i*2] = sample;
		buffer[i*2+1] = sample+=500;
	}
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Init(JNIEnv*, jobject)
{
	LOGD("Init...");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    texture = new AssetTextureClass(vm);
    int ret = texture->AddTexture("images/ascii_font.png");
    if(ret == -1) LOGE("Fehler beim laden der Textur !");

    textBox01 = new TextBoxClass(2.0f,2.0f,60,30);
    textBox01->SetFont(texture->GetTexturID(ret),16,16);
    textBox01->SetPos(-1.0f,-1.0f);
    text_buffer = textBox01->GetTextBuffer();

    textBox01->SetText(0,0,"Sound Test by Thorsten Kattanek");

    OpenSLWrap_Init(SoundCallback);
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Resize(JNIEnv*, jobject, jint xw, jint yw)
{
    glViewport(0,0,xw,yw);
    glMatrixMode(GL_PROJECTION);
    glOrthof(0,xw,yw,0,-1,1);
    glLoadIdentity();
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Renderer(JNIEnv*, jobject)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	textBox01->Renderer();
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Pause(JNIEnv*, jobject)
{
	LOGD("PAUSE...");
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Resume(JNIEnv*, jobject)
{
	LOGD("RESUME...");
}

JNIEXPORT void JNICALL Java_de_kattisoft_emu64_NativeClass_Destroy(JNIEnv*, jobject)
{
	LOGD("Destroy...");

	OpenSLWrap_Shutdown();
	if(textBox01 != NULL) delete textBox01;
	if(texture != NULL) delete texture;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* _vm, void* reserved)
{
	vm = _vm;
	LOGD("OnLoad...");

	return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
