/* I05: select and upgrade owned vehicles by their exact native storage identity. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
#include "transaction.h"
#include "controls.h"
static void render(PTR menu){
 if(transaction_unknown){text(member(menu,"summary"),unknown_notice);return;}
 if(!count){text(member(menu,"summary"),"GARAGE MANAGEMENT\nNo vehicles owned by this player.\nBuy a vehicle from the Vehicles page.");return;}
 if(selected<0||selected>=count){text(member(menu,"summary"),"GARAGE MANAGEMENT\nSelect an owned Fleet record or use Alt+PageUp / PageDown.\nNo replacement vehicle is selected automatically.");return;}
 PTR owned=dget(records[selected]);int ok=1,model=int_field(owned,"vehicle"),garage=int_field(owned,"garage");void *home_args[]={owned};PTR home=call(home_m,dget(world_root),home_args,&ok);if(home&&ok)garage=garage_int(home,&ok);char b[2500],*p=cat(b,"MANAGED VEHICLE ");p=num(p,(U64)selected+1);p=cat(p," / ");p=num(p,(U64)count);p=cat(p,"\nExact storage: ");p=string_text(p,dget(selection),160);p=cat(p,"\nOwner: ");p=string_text(p,member(owned,"ownerName"),80);p=cat(p,"  |  Model #");p=num(p,(U64)(model>=0?model:0));p=cat(p,"  |  Garage #");if(garage<0){p=cat(p,"-");p=num(p,(U64)-(long long)garage);}else p=num(p,(U64)garage);p=cat(p,int_field(owned,"inWorld")?"\nDeployed\n":"\nStored\n");
 p=cat(p,"Alt+PgUp / PgDn select exact owned vehicle\nAlt+1..7 twice: upgrade THIS storage identity\n");
 for(int i=0;i<7;i++){
  int kind=kinds[i];void *a[]={owned,&kind};PTR l=call(level_m,0,a,&ok);int level=garage_int(l,&ok);p=num(p,(U64)i+1);p=cat(p," ");p=cat(p,labels[i]);p=cat(p," L");p=num(p,(U64)(level>0?level:0));
  unsigned char price[16]={0};void *pa[]={owned,&kind,price};int available=garage_bool(call(price_m,0,pa,&ok),&ok);
  if(ok&&available){void *da_args[]={price};PTR formatted=call(price_text_m,0,da_args,&ok);p=cat(p,"  ");p=string_text(p,formatted,50);}else p=cat(p,"  [max / unavailable]");p=cat(p,"\n");
 }
 if(armed>=0){void *a[]={armed_price};PTR formatted=call(price_text_m,0,a,&ok);p=cat(p,"\nCONFIRM ");p=cat(p,labels[armed]);p=cat(p," L");p=num(p,(U64)armed_level);p=cat(p," -> L");p=num(p,(U64)armed_level+1);p=cat(p," for ");p=string_text(p,formatted,50);p=cat(p,"\n");}
 p=cat(p,"\n");p=cat(p,status);p=cat(p,"\n");string_text(p,dget(result_message),240);text(member(menu,"summary"),b);
}
static int before(int id,Context *c){
 (void)id;if(!on_thread())return 0;PTR walker=(PTR)c->rcx;
 if(!garage_local_admit(walker))return 0;
 PTR world=0;field_value(walker,world_f,&world);PTR menu=garage_menu(walker);int ok=1;
 int page=menu?int_field(menu,"page"):-1;
 int active=world&&menu&&(page==upgrades_page||page==fleet_page)&&!garage_bool(call(restoring_m,world,0,&ok),&ok)&&ok;
 if(!active){reset();}else if(world!=dget(world_root)||walker!=dget(actor_root)||menu!=dget(menu_root)){
  reset();world_root=dhold(world);actor_root=dhold(walker);menu_root=dhold(menu);
  if(!world_root||!actor_root||!menu_root){reset();active=0;}
 }
 int input_allowed=active&&input_ok();
 GarageActions actions;garage_actions(input_allowed,&actions);
 if(!active)return 0;
 if(transaction_unknown){render(menu);return 0;}
 if(page!=active_page){disarm();active_page=page;}
 float now=0;if(!garage_value(call(time_m,0,0,&ok),&now,sizeof(now),&ok)||!garage_clock(now)){disarm();return 0;}
 /* Both Fleet and Upgrades use the native picked storage, including menu reopen. */
 PTR picked=member(menu,"fleetPicked");if(picked!=dget(native_pick)&&!same(picked,dget(native_pick))){
  ddrop(&native_pick);native_pick=dhold(picked);ddrop(&selection);selection=native_pick?dhold(dget(native_pick)):0;
  disarm();ddrop(&result_message);refresh_at=0;
 }
 if(now>=refresh_at){refresh(world);refresh_at=now+.5f;}
 if(!input_allowed||now>deadline)disarm();
 if(input_allowed&&count&&(actions.previous.pressed||actions.next.pressed)){
  int target=selected<0?(actions.previous.pressed?count-1:0):(selected+(actions.previous.pressed?count-1:1))%count;
  PTR key_obj=call(mf.storage,dget(records[target]),0,&ok);DRoot key_root=ok?dhold(key_obj):0;
  if(key_root){void *args[]={dget(key_root)};call(pick_m,menu,args,&ok);
   if(ok){ddrop(&selection);selection=key_root;key_root=0;ddrop(&native_pick);native_pick=dhold(dget(selection));selected=target;status="Selected exact owned vehicle.";}
   ddrop(&key_root);disarm();ddrop(&result_message);
  }
 }
 if(input_allowed)for(int i=0;i<7;i++)if(actions.upgrades[i].pressed){upgrade(world,i,now);break;}
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
