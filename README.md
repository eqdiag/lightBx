# lightBx (Lighting basics)

Basic lighting system implemented in Vulkan.

Roughly based on lighting section in [learnopengl](https://learnopengl.com/Lighting).



## Dependencies
- vk_bootstrap: For vulkan boiler plate (Instance, Physical Device, Device creation)
- vma: Vulkan memory allocator
- tiny_obj: Loading 3d model obj files
- stb_image: Loading image files

All other dependencies are self-contained in this project using git's submodule system.


## Installation

```
git clone https://github.com/eqdiag/lightBx
cd lightBx
mkdir build
cd build
cmake ..
make
```

## TODO
- [x] Setup
	- [x] Flashing screen w changing color
- [ ] Hardcoded triangle
	- [ ] Pipeline abstraction
	- [ ] Shaders done
- [ ] Test w 3d triangle (via vertex buffers)
	- [ ] Depth buffer
- [ ] Mesh loading
- [ ] Fly Cam
- [ ] Imgui
- [ ] Colors
- [ ] Basic lighting
- [ ] Materials
- [ ] Light casters
- [ ] Multiple lights


## Keyboard Controls
  * `Placeholder` TEXT


## Mouse Controls
  * `Placeholder` TEXT

# Demo
## Placeholder
![Control Demo](/screenshots/control_demo.gif "Control Demo")

