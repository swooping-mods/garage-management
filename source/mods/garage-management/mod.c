/* I05: select and upgrade owned vehicles by their exact native storage identity. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
static PTR home_m,local_m,pick_m,level_m,price_m,upgrade_m,price_text_m,joined_m,host_m,steam_m;
static PTR (*nested)(PTR,PTR *);static const char *(*class_name)(PTR);
static DRoot records[128],world_root,selection,armed_key;static int count,selected,armed=-1,armed_level,world_authority,kinds[7];static float refresh_at,deadline;static const char *status="";static DRoot result_message;
static unsigned char armed_price[16];
static int authority(void);
static const char *labels[]={"Speed","Hopper","Turbo","Cruise","Suction","Unload","Lights"};
static void clear_records(void){for(int i=0;i<count;i++)ddrop(&records[i]);count=0;}
static void reset(void){clear_records();ddrop(&world_root);ddrop(&selection);ddrop(&armed_key);ddrop(&result_message);armed=-1;world_authority=0;refresh_at=0;status="";}
static int same(PTR a,PTR b){return mf_equal(a,b);}
static void refresh(PTR world){
 clear_records();int ok=1;PTR local=call(local_m,0,0,&ok);MoonFleetSnapshot snapshot={0};
 if(!ok||!local||!mf_snapshot(world,&snapshot))return;
 for(unsigned i=0;i<snapshot.count&&count<128;i++){
  PTR owned=mf_item(&snapshot,i);MoonFleetRecord record={0};
  if(mf_read(owned,&record)&&moon_fleet_local(&mf_queries,&record,local)){
   DRoot retained=dhold(owned);if(retained)records[count++]=retained;
  }
 }
 mf_close(&snapshot);selected=0;
 for(int i=0;i<count;i++){PTR key_obj=call(mf.storage,dget(records[i]),0,&ok);if(same(key_obj,dget(selection))){selected=i;break;}}
 if(count){PTR key_obj=call(mf.storage,dget(records[selected]),0,&ok);ddrop(&selection);selection=dhold(key_obj);}
}
static int authority(void){int ok=1;if(!boolean(joined_m,0,&ok))return ok;PTR a=call(host_m,0,0,&ok),b=call(steam_m,0,0,&ok);return ok&&a&&b&&*(U64*)unbox(a)==*(U64*)unbox(b);}
static void upgrade(PTR world,int slot,float now){
 if(!count||!world_authority||!authority()){status="Only the world host can install these upgrades.";return;}
 PTR owned=dget(records[selected]);int kind=kinds[slot],ok=1;void *la[]={owned,&kind};PTR boxed=call(level_m,0,la,&ok);int level=boxed?*(int*)unbox(boxed):-1;unsigned char price[16]={0};void *pa[]={owned,&kind,price};PTR available=call(price_m,0,pa,&ok);
 if(!ok||level<0||!available||!*(unsigned char*)unbox(available)){status="Upgrade unavailable or maximum tier reached.";armed=-1;return;}
 PTR key_obj=call(mf.storage,owned,0,&ok);int unchanged=armed_level==level;for(unsigned i=0;i<16;i++)if(armed_price[i]!=price[i])unchanged=0;
 if(armed!=slot||!same(dget(armed_key),key_obj)||now>deadline||!unchanged){armed=slot;armed_level=level;dcopy(armed_price,price,16);deadline=now+5;ddrop(&armed_key);armed_key=dhold(key_obj);status="Press the same Alt+number again within 5 seconds to confirm.";return;}
 PTR player=call(local_m,0,0,&ok),message=0;void *args[]={player,key_obj,&kind,&level,&message};PTR accepted=call(upgrade_m,world,args,&ok);
 ddrop(&result_message);result_message=dhold(message);status=ok&&accepted&&*(unsigned char*)unbox(accepted)?"Upgrade installed on this exact vehicle.":"Upgrade refused by game.";armed=-1;ddrop(&armed_key);event(ok?"exact-storage-upgrade-result":"exact-storage-upgrade-error");
}
static void render(PTR menu){
 if(!count){text(member(menu,"summary"),"GARAGE MANAGEMENT\nNo vehicles owned by this player.\nBuy a vehicle from the Vehicles page.");return;}
 PTR owned=dget(records[selected]);int ok=1,model=int_field(owned,"vehicle"),garage=int_field(owned,"garage");void *home_args[]={owned};PTR home=call(home_m,dget(world_root),home_args,&ok);if(home)garage=*(int*)unbox(home);char b[2500],*p=cat(b,"MANAGED VEHICLE ");p=num(p,(U64)selected+1);p=cat(p," / ");p=num(p,(U64)count);p=cat(p,"\nExact storage: ");p=string_text(p,dget(selection),160);p=cat(p,"\nOwner: ");p=string_text(p,member(owned,"ownerName"),80);p=cat(p,"  |  Model #");p=num(p,(U64)(model>=0?model:0));p=cat(p,"  |  Garage #");if(garage<0){p=cat(p,"-");p=num(p,(U64)-(long long)garage);}else p=num(p,(U64)garage);p=cat(p,int_field(owned,"inWorld")?"\nDeployed\n":"\nStored\n");
 p=cat(p,"Alt+PgUp / PgDn select exact owned vehicle\nAlt+1..7 twice: upgrade THIS storage identity\n");
 for(int i=0;i<7;i++){
  int kind=kinds[i];void *a[]={owned,&kind};PTR l=call(level_m,0,a,&ok);int level=l?*(int*)unbox(l):0;p=num(p,(U64)i+1);p=cat(p," ");p=cat(p,labels[i]);p=cat(p," L");p=num(p,(U64)(level>0?level:0));
  unsigned char price[16]={0};void *pa[]={owned,&kind,price};PTR available=call(price_m,0,pa,&ok);
  if(available&&*(unsigned char*)unbox(available)){void *da_args[]={price};PTR formatted=call(price_text_m,0,da_args,&ok);p=cat(p,"  ");p=string_text(p,formatted,50);}else p=cat(p,"  [max / unavailable]");p=cat(p,"\n");
 }
 p=cat(p,"\n");p=cat(p,status);p=cat(p,"\n");string_text(p,dget(result_message),240);text(member(menu,"summary"),b);
}
static int before(int id,Context *c){
 (void)id;if(!on_thread())return 0;PTR walker=(PTR)c->rcx,world=0;field_value(walker,world_f,&world);if(world!=dget(world_root)){reset();if(world){world_root=dhold(world);world_authority=authority();}}PTR menu=garage_menu(walker);
 int input_allowed=menu&&input_ok();
 MoonInputState up_action=moon_input_action("garage-management.previous","Alt+PageUp",input_allowed);
 MoonInputState down_action=moon_input_action("garage-management.next","Alt+PageDown",input_allowed);
 MoonInputState upgrades[7];static const char *action_names[]={"garage-management.speed","garage-management.hopper","garage-management.turbo","garage-management.cruise","garage-management.suction","garage-management.unload","garage-management.lights"};
 for(int i=0;i<7;i++){char chord[]={ 'A','l','t','+',(char)('1'+i),0};upgrades[i]=moon_input_action(action_names[i],chord,input_allowed);}

 if(!menu){armed=-1;ddrop(&armed_key);return 0;}
 int page=int_field(menu,"page");if(page!=upgrades_page&&page!=fleet_page)return 0;int ok=1;float now=dnumber(time_m,0,&ok);if(!ok)return 0;
 if(!selection){PTR picked=member(menu,"fleetPicked");if(picked)selection=dhold(picked);}
 if(now>=refresh_at){refresh(world);refresh_at=now+.5f;}
 /* Follow the vanilla fleet row when the operator clicks a different record. */
 if(page==fleet_page){PTR picked=member(menu,"fleetPicked");if(picked&&!same(picked,dget(selection))){for(int i=0;i<count;i++){PTR key_obj=call(mf.storage,dget(records[i]),0,&ok);if(same(picked,key_obj)){selected=i;ddrop(&selection);selection=dhold(key_obj);armed=-1;break;}}}}
 int allow=input_allowed,up=up_action.pressed,dn=down_action.pressed,slot=0;
 if(count&&(up||dn)){selected=(selected+(up?count-1:1))%count;PTR key_obj=call(mf.storage,dget(records[selected]),0,&ok);ddrop(&selection);selection=dhold(key_obj);void *args[]={key_obj};call(pick_m,menu,args,&ok);armed=-1;status="Selected exact owned vehicle.";ddrop(&result_message);}
 for(int i=0;i<7;i++)if(upgrades[i].pressed){slot=i+1;break;}
 if(slot)upgrade(world,slot-1,now);if(!allow||now>deadline)armed=-1;
 render(menu);return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
 if(!bind_base(domain,game,"garage-management.jsonl")||!mf_bind(game,0))return 0;
 nested=(PTR(*)(PTR,PTR*))GetProcAddress(game,"il2cpp_class_get_nested_types");class_name=(const char*(*)(PTR))GetProcAddress(game,"il2cpp_class_get_name");if(!nested||!class_name)return 0;
 PTR outer=find_class("MoonCleaners","MoonVehicleUpgrades"),it=0,k,kind_class=0;while(outer&&(k=nested(outer,&it)))if(deq(class_name(k),"Kind"))kind_class=k;if(!kind_class)return 0;
 for(int i=0;i<7;i++){PTR f=get_field(kind_class,labels[i]);if(!f)return 0;static_field(f,&kinds[i]);}
#define B(F,NS,T,N,S) F=dmethod(NS,T,N,S);if(!F)return 0
 B(home_m,"MoonCleaners","MoonWorld","HomeGarage","MoonCleaners.MoonOwnedVehicle");B(local_m,"MoonCleaners","MoonCrew","get_LocalPlayerId","");B(pick_m,"MoonCleaners","MoonGarageMenu","PickFleet","System.String");
 B(level_m,"MoonCleaners","MoonWorld","VehicleUpgradeLevel","MoonCleaners.MoonOwnedVehicle;MoonCleaners.MoonVehicleUpgrades.Kind");B(price_m,"MoonCleaners","MoonWorld","TryGetVehicleUpgradePrice","MoonCleaners.MoonOwnedVehicle;MoonCleaners.MoonVehicleUpgrades.Kind;System.Decimal&");B(upgrade_m,"MoonCleaners","MoonWorld","TryUpgradeVehicle","System.String;System.String;MoonCleaners.MoonVehicleUpgrades.Kind;System.Int32;System.String&");B(price_text_m,"MoonCleaners","MoonGarageMenu","Price","System.Decimal");
 B(joined_m,"MoonCleaners","MoonCrew","get_Joined","");B(host_m,"MoonCleaners","MoonCrew","get_HostId","");B(steam_m,"Steamworks","SteamUser","GetSteamID","");
#undef B
 if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;event("ready-exact-owned-selection");return install_hooks();
}
