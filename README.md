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
- [x] Hardcoded triangle
	- [x] Pipeline abstraction
	- [x] Shaders done
- [x] Fly Cam
	- [x] Uniform buffer for proj,view
	- [x] Add camera layer on top to actually compute view matrix
- [x] Vertex buffers 
- [x] Storage buffer for model matrices of objects
- [x] Depth buffer


# Thursday
- [x] Static buffer loading (via staging buffer)
- [x] Index buffer (squares)
- [x] Index buffer (cubes)

- [ ] Imgui
- [ ] Colors
- [ ] Basic lighting
- [ ] Fix mac bug (framebuffer issue vs window size)

# Friday
- [ ] Materials
- [ ] Textures on cubes
	- [ ] texture loading
- [ ] Light casters
- [ ] Multiple lights


## Keyboard Controls
  * `W` Translate camera forward
  * `A` Translate camera left
  * `S` Translate camera back
  * `D` Translate camera right


## Mouse Controls
  * `Click + Drag` Rotate camera forward direction

# Demo
## Placeholder
![Control Demo](/screenshots/control_demo.gif "Control Demo")


