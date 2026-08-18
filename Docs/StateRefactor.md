

## Cards

### ProDOS Clock

This is currently using cpu as context. It really needs the mmu in order to write the time into memory.

### Speaker

softswitches easily changed to use the module state. However, audio_generate_frame needs to be called with cpu cycle counters, and from the "generate_frame" part of the main loop.

### ThunderClock

I think this can be changed as-is to use its slot state as context.

### gamecontroller

Gamecontroller uses cpu->cycles quite extensively.

### display

display currently handles all the apple ii video modes but also vectors off to the Videx code based on the annunciator value. gonna have to give this one some thought. probably the regular apple ii display shouldn't have to know about videx, or vice-versa, and of course in future there are going to be GS SHR and maybe Apple III modes etc.

### annunciator

this can be 100% local context, however, other modules need to know the state of the annunciator. So this is where annunciator would "publish" its status somehow for other code to read. (The "module_store" is -sort of- used like that now. But should be made a little more formal.)

### parallel

ready to change to use slot state as context.

### mem expansion

this is ready to go except it also needs mmu for mapping C8XX space

### videx 
needs mmu for mapping its video ram space in C8xx from a softswitch. MMU can be injected into device state. other videx code (line rendering etc) needs access to videosystem.

### mockingboard
needs access to "audio system". also needs to set interrupts. (in reality, all devices have some potential to set interrupts.)
currently does interrupts via cpu. also uses eventtimer in cpu.

### languagecard 
of course about the only thing this does is manipulate the mmu.


## Summary

So, in aggregate, these devices need to access:

computer
mmu
cpu (for cycle counts)
videosystem

They all need a consistent interface to be passed these values, even if they don't need to use them. I guess that's "computer".
(Unless and to extent they can be made to rely on "published" information instead.)

Now does this actually result in decoupling? if I have to pass a struct with all of them? Give some thought.

computer is the container for all these things. just pass computer - that gives mmu, cpu, etc. But, don't have the various objects back-link to container. (or, have computer only use short prototypes). or, computer can store void *'s and we can typecast values as we fetch them from a typeless container. That's the bottom line - we need some kind of typeless storage. We had that with the slot_storage and module_storage. Can provide some macros perhaps to fetch these things from computer. Then each device just knows computer. (or, perhaps, it knows a single method that can fetch the object it wants).
Maybe they only use these macros in their constructors to get exactly the values they need and put them in their own structs.

Something like:

Can these be implemented in the header so we don't have to suck in the referenced headers?
computer->set_system_module(SYS_MODULE_ID, void *)
void *computer->get_system_module(SYS_MODULE_ID)

GET_SYS_MODULE(OBJECT_TYPE, SYS_MODULE_ID) ((OBJECT_TYPE *)computer->get_system_module(SYS_MODULE_ID))
