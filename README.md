# lightBx (Lighting basics)

Basic lighting system implemented in Vulkan.

Roughly based on lighting section in [learnopengl](https://learnopengl.com/Lighting).


## Learning goals
- [x] Implement instanced rendering 
- [x] Distance attenuated point lights (phong shading)
- [x] Use multiple vulkan graphics pipelines
- [x] Practice using vulkan dynamic descriptor sets for uniform and storage buffers
- [x] Practice using vulkan push constants 


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
git submodule update --init

mkdir build
cd build
cmake ..
make
```



## Keyboard Controls
  * `W` Translate camera forward
  * `A` Translate camera left
  * `S` Translate camera back
  * `D` Translate camera right


## Mouse Controls
  * `Click + Drag` Rotate camera forward direction

# Demos
![RGB Demo](/screenshots/rgb.gif "RGB")
![Control Demo](/screenshots/control.gif "Control")

