/* Managed references kept between frames belong to the current local menu. */
static PTR home_m,local_m,pick_m,level_m,price_m,upgrade_m,price_text_m,joined_m,host_m,steam_m;
static PTR (*nested)(PTR,PTR *);static const char *(*class_name)(PTR);
static DRoot records[128],world_root,actor_root,menu_root,selection,native_pick,armed_key,armed_player;
static int count,selected=-1,armed=-1,armed_level,kinds[7];
static int active_page=-1;static float refresh_at,deadline;static const char *status="";static DRoot result_message;
static unsigned char armed_price[16];
/* A partial native payment/install cannot be retried safely. Only a new process
 * clears this latch; menu/world cleanup deliberately leaves it untouched. */
static int transaction_unknown,clock_seen;static float clock_last;
static const char *unknown_notice="Upgrade outcome unknown. Further mod upgrades are disabled until game restart. Inspect this vehicle and wallet before continuing.";
static const char *labels[]={"Speed","Hopper","Turbo","Cruise","Suction","Unload","Lights"};
static void disarm(void){armed=-1;deadline=0;ddrop(&armed_key);ddrop(&armed_player);}
static void clear_records(void){for(int i=0;i<count;i++)ddrop(&records[i]);count=0;selected=-1;}
static void reset(void){clear_records();ddrop(&world_root);ddrop(&actor_root);ddrop(&menu_root);ddrop(&selection);ddrop(&native_pick);disarm();ddrop(&result_message);refresh_at=0;active_page=-1;clock_seen=0;clock_last=0;status=transaction_unknown?unknown_notice:"";}
static int garage_value(PTR boxed,void *out,U64 size,int *ok){
 PTR data=*ok&&boxed?unbox(boxed):0;if(!data){*ok=0;return 0;}dcopy(out,data,size);return 1;
}
static int garage_bool(PTR boxed,int *ok){
 unsigned char value=0;if(!garage_value(boxed,&value,1,ok)||value>1){*ok=0;return 0;}return value;
}
static int garage_int(PTR boxed,int *ok){int value=-1;garage_value(boxed,&value,sizeof(value),ok);return value;}
static int garage_clock(float now){
 /* Ordered comparisons reject NaN and both infinities without a CRT dependency. */
 if(!(now>=0&&now<=3.402823466e38F)){disarm();refresh_at=0;status="Upgrade confirmation cancelled: invalid clock.";return 0;}
 if(clock_seen&&now<clock_last){disarm();refresh_at=0;clock_last=now;status="Upgrade confirmation cancelled: clock moved backwards.";return 0;}
 clock_seen=1;clock_last=now;return 1;
}
static int same(PTR a,PTR b){return a&&b&&mf_equal(a,b);}
static void refresh(PTR world){
 clear_records();int ok=1;DRoot local=dhold(call(local_m,0,0,&ok));MoonFleetSnapshot snapshot={0};
 if(!ok||!local||!mf_snapshot(world,&snapshot)){ddrop(&local);disarm();return;}
 for(unsigned i=0;i<snapshot.count&&count<128;i++){
  PTR owned=mf_item(&snapshot,i);MoonFleetRecord record={0};
  if(mf_read(owned,&record)&&moon_fleet_local(&mf_queries,&record,dget(local))){
   DRoot retained=dhold(owned);if(retained)records[count++]=retained;
  }
 }
 mf_close(&snapshot);ddrop(&local);
 /* A vanished identity leaves no target; never substitute the first same-model car. */
 for(int i=0;i<count;i++){PTR key_obj=call(mf.storage,dget(records[i]),0,&ok);if(ok&&same(key_obj,dget(selection))){if(selected>=0){selected=-1;break;}selected=i;}}
 if(selected<0){disarm();ddrop(&selection);}
}
static int authority(void){
 int ok=1;int joined=garage_bool(call(joined_m,0,0,&ok),&ok);if(!ok)return 0;if(!joined)return 1;
 U64 host=0,steam=0;garage_value(call(host_m,0,0,&ok),&host,sizeof(host),&ok);
 garage_value(call(steam_m,0,0,&ok),&steam,sizeof(steam),&ok);return ok&&host&&steam&&host==steam;
}
/* Check the complete fresh snapshot before using native lookup. An ambiguous or
 * incomplete snapshot cannot authorize an arbitrary instance with a reused key. */
static DRoot current_owned(PTR world,PTR storage,PTR player){
 MoonFleetSnapshot snapshot={0};if(!storage||!player||!mf_snapshot(world,&snapshot))return 0;
 DRoot found=0;int matches=0,valid=1;
 for(unsigned i=0;i<snapshot.count;i++){
  MoonFleetRecord record={0};if(!mf_read(mf_item(&snapshot,i),&record)){valid=0;break;}
  if(same(record.storage,storage)){
   matches++;if(matches==1&&moon_fleet_local(&mf_queries,&record,player))found=dhold(record.record);
  }
 }
 int ok=1;if(valid&&matches==1&&found){PTR lookup=mf_lookup(world,storage,&ok);if(!ok||lookup!=dget(found))valid=0;}else valid=0;
 mf_close(&snapshot);if(!valid)ddrop(&found);return found;
}
static void upgrade(PTR world,int slot,float now){
 if(transaction_unknown){disarm();status=unknown_notice;return;}if(!garage_clock(now))return;
 if(slot<0||slot>=7||selected<0||selected>=count||!world||!authority()){disarm();status="Only the world host can install upgrades on a selected vehicle.";return;}
 int ok=1;DRoot player=dhold(call(local_m,0,0,&ok));
 DRoot owned=ok&&player?current_owned(world,dget(selection),dget(player)):0;MoonFleetRecord record={0};
 if(!ok||!player||!owned||!mf_read(dget(owned),&record)||!moon_fleet_local(&mf_queries,&record,dget(player))){
  disarm();status="Selected vehicle is no longer owned by this player.";goto done;
 }
 int kind=kinds[slot];void *la[]={dget(owned),&kind};PTR boxed=call(level_m,0,la,&ok);int level=garage_int(boxed,&ok);
 unsigned char price[16]={0};void *pa[]={dget(owned),&kind,price};int available=garage_bool(call(price_m,0,pa,&ok),&ok);
 if(!ok||level<0||!available){disarm();status="Upgrade unavailable or maximum tier reached.";goto done;}
 int unchanged=armed_level==level;for(unsigned i=0;i<16;i++)if(armed_price[i]!=price[i])unchanged=0;
 if(armed!=slot||!same(dget(armed_key),dget(selection))||!same(dget(armed_player),dget(player))||now>deadline||!unchanged){
  disarm();armed_key=dhold(dget(selection));armed_player=dhold(dget(player));
  if(!armed_key||!armed_player){disarm();status="Could not retain the selected upgrade identity.";goto done;}
  float until=now+5;if(!(until>now&&until<=3.402823466e38F)){disarm();status="Upgrade confirmation cancelled: invalid clock range.";goto done;}
  armed=slot;armed_level=level;dcopy(armed_price,price,16);deadline=until;
  ddrop(&result_message);status="Press the same Alt+number again within 5 seconds to confirm.";goto done;
 }
 /* Consume before entering native code, including refusals/exceptions. */
 PTR message=0;void *args[]={dget(player),dget(selection),&kind,&level,&message};disarm();
 int accepted=garage_bool(call(upgrade_m,world,args,&ok),&ok);ddrop(&result_message);
 if(!ok){transaction_unknown=1;status=unknown_notice;event("exact-storage-upgrade-outcome-unknown");}
 else{result_message=dhold(message);status=accepted?"Upgrade installed on this exact vehicle.":"Upgrade refused by game.";event("exact-storage-upgrade-result");}
 refresh_at=0;
 done:ddrop(&owned);ddrop(&player);
}
