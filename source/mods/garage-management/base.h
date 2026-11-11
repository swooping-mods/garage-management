/* Each package compiles its own reflection helpers; no feature DLL dependency. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/input.h"
#include "api.h"
int _fltused=0;
static PTR component_m,menu_open_m,text_m,world_f,focus_m,typing_m,time_m;
static void (*static_field)(PTR,void *);
static const unsigned short *(*string_chars)(PTR);static int (*string_length)(PTR);
/* Retain the legacy high-bit mask while honoring shared input admission. */
static short key(int vk){return moon_input_key((unsigned)vk)?(short)0x8000:0;}
static PTR (*foreground)(void);static DWORD (*window_pid)(PTR,DWORD *);
static int vehicles_page,upgrades_page,fleet_page;
static PTR log_file;static DWORD owner_thread;
static void event(const char *s){if(!log_file)return;char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(log_file,b,(DWORD)(p-b),&n,0);}
static PTR member(PTR object,const char *name){PTR value=0;if(object){PTR f=get_field(object_class(object),name);if(f)field_value(object,f,&value);}return value;}
static int int_field(PTR object,const char *name){int value=0;if(object){PTR f=get_field(object_class(object),name);if(f)field_value(object,f,&value);}return value;}
static float float_field(PTR object,const char *name){float value=0;if(object){PTR f=get_field(object_class(object),name);if(f)field_value(object,f,&value);}return value;}
static char *string_text(char *p,PTR string,unsigned limit){if(string){int n=string_length(string);const unsigned short *s=string_chars(string);for(int i=0;i<n&&limit;i++,limit--)*p++=s[i]>=32&&s[i]<127?(char)s[i]:'?';}*p=0;return p;}
static char *decimal_text(char *p,float value){if(value<0){*p++='-';value=-value;}if(value>10000000)return cat(p,"?");unsigned n=(unsigned)(value*10+.5f);p=num(p,n/10);*p++='.';*p++=(char)('0'+n%10);*p=0;return p;}
static void text(PTR target,const char *s){if(!target)return;int ok=1;void *a[]={da.string(s)};call(text_m,target,a,&ok);}
static int down(int vk){return (key(vk)&0x8000)!=0;}
static PTR garage_menu(PTR walker){int ok=1;void *a[]={dtype("MoonCleaners","MoonGarageMenu")};PTR menu=call(component_m,walker,a,&ok);return ok&&menu&&boolean(menu_open_m,menu,&ok)&&ok?menu:0;}
static int input_ok(void){int ok=1;DWORD pid=0;window_pid(foreground(),&pid);return pid==GetCurrentProcessId()&&boolean(focus_m,0,&ok)&&!boolean(typing_m,0,&ok)&&ok;}
static int on_thread(void){DWORD id=GetCurrentThreadId();if(!owner_thread)owner_thread=id;return id==owner_thread;}
static int bind_base(PTR domain,PTR game,const char *logname){
 if(!bind_runtime(domain,game)||!dapi(game))return 0;
 static_field=(void(*)(PTR,void*))GetProcAddress(game,"il2cpp_field_static_get_value");string_chars=(const unsigned short*(*)(PTR))GetProcAddress(game,"il2cpp_string_chars");string_length=(int(*)(PTR))GetProcAddress(game,"il2cpp_string_length");
 PTR user=LoadLibraryA("user32.dll");foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");window_pid=(DWORD(*)(PTR,DWORD*))GetProcAddress(user,"GetWindowThreadProcessId");
#define B(F,NS,T,N,S) F=dmethod(NS,T,N,S);if(!F)return 0
 B(component_m,"UnityEngine","Component","GetComponent","System.Type");B(menu_open_m,"MoonCleaners","MoonGarageMenu","get_IsOpen","");B(text_m,"TMPro","TMP_Text","set_text","System.String");B(focus_m,"UnityEngine","Application","get_isFocused","");B(typing_m,"MoonCleaners","MoonInput","get_Typing","");B(time_m,"UnityEngine","Time","get_unscaledTime","");
#undef B
 PTR (*nested_type)(PTR,PTR*)=(PTR(*)(PTR,PTR*))GetProcAddress(game,"il2cpp_class_get_nested_types");const char *(*type_name)(PTR)=(const char*(*)(PTR))GetProcAddress(game,"il2cpp_class_get_name");
 if(!nested_type||!type_name)return 0;PTR it=0,k,page=0,outer=find_class("MoonCleaners","MoonGarageMenu");while(outer&&(k=nested_type(outer,&it)))if(deq(type_name(k),"Page"))page=k;
 if(!page||!static_field)return 0;PTR pf=get_field(page,"Vehicles"),pu=get_field(page,"Upgrades"),pl=get_field(page,"Fleet");if(!pf||!pu||!pl)return 0;static_field(pf,&vehicles_page);static_field(pu,&upgrades_page);static_field(pl,&fleet_page);
 world_f=get_field(find_class("MoonCleaners","MoonWalker"),"world");
 char path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,900);if(n&&n<900){cat(path+n,"\\");cat(path+n+1,logname);log_file=CreateFileA(path,0x40000000,1,0,2,0x80,0);if(log_file==(PTR)(U64)-1)log_file=0;}
 (void)integer;(void)num;(void)method;(void)int_field;(void)float_field;(void)decimal_text;(void)string_text;(void)input_ok;(void)member;(void)text;(void)garage_menu;(void)down;
 return static_field&&string_chars&&string_length&&foreground&&window_pid&&world_f;
}
