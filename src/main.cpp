#include "core/vk_app.h"


int main(){

	VkApp app{};

	app.init();

	app.run();

	app.cleanup();

	return 0;
}