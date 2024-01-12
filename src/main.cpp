#include "core/vk_engine.h"


int main(){

    VkEngine engine;

    engine.init();

    engine.run();
    
    engine.cleanup();

    return 0;
}