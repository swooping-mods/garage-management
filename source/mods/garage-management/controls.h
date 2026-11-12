/* Sample every action while gated to consume held edges through actor loss. */
typedef struct {MoonInputState previous,next,upgrades[7];} GarageActions;
static void garage_actions(int allowed,GarageActions *actions){
 actions->previous=moon_input_action("garage-management.previous","Alt+PageUp",allowed);
 actions->next=moon_input_action("garage-management.next","Alt+PageDown",allowed);
 static const char *names[]={"garage-management.speed","garage-management.hopper","garage-management.turbo","garage-management.cruise","garage-management.suction","garage-management.unload","garage-management.lights"};
 for(int i=0;i<7;i++){char chord[]={'A','l','t','+',(char)('1'+i),0};actions->upgrades[i]=moon_input_action(names[i],chord,allowed);}
}
static int garage_local_admit(PTR walker){
 PTR local=0;if(!moon_local_actor_read(&local)||!local){GarageActions ignored;garage_actions(0,&ignored);reset();return 0;}
 return walker==local;
}
