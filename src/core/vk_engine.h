#pragma once

#include "core/vk_init.h"
#include "vk_init.h"
#include <vector>
#include "vk_mesh.h"
#include "math/matrix.h"
#include <functional>

constexpr unsigned int NUM_FRAMES = 2;
constexpr unsigned int MAX_OBJECTS = 1000;

struct Texture {
    AllocatedImage image;
    VkImageView imageView;
};

//For cpu -> resource copying
struct UploadContext {
    VkFence _uploadFence;
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;
};

struct GPUObjectData {
    math::Mat4 modelMatrix;
};

struct GPUSceneData {
    math::Vec4 fogColor;
    math::Vec4 fogDistances;
    math::Vec4 ambientColor;
    math::Vec4 sunlightDir;
    math::Vec4 sunlightColor;
};

struct GPUCameraData {
    math::Mat4 view;
    math::Mat4 proj;
    math::Mat4 viewproj;
};

struct MeshPushConstants {
    math::Vec4 data;
    math::Mat4 matrix;
};

struct FrameData {
    VkSemaphore _imgReadyFlag;
    VkSemaphore _renderDoneFlag;
    VkFence _renderDone;

    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;

    //Camera view/proj stuff
    AllocatedBuffer _cameraBuffer;
    VkDescriptorSet _globalDescriptor;


    //Object buffer (for all objects)
    AllocatedBuffer _objectBuffer;
    VkDescriptorSet _objectDescriptorSet;
};

class VkEngine{

public:

    void init();
    void run();
    void draw();
    void cleanup();

    /* Mem allocator */
    VmaAllocator _allocator;

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);


private:

    void initWindow();

    void initVulkan();

    void initSwapchain();

    void initCommands();

    void initRenderPass();

    void initFramebuffers();

    void initSync();

    void initDescriptors();

    void initPipelines();

    void loadMeshes();

    void loadImages();

    void initScene();

    void uploadMesh(Mesh& mesh);


    FrameData& getCurrentFrame();

    /* App State */

    bool _init{false};
    uint32_t _frameNum{0};
    int _gridSize = 10;
    float _gridSpacing = 2.0f;

    /* Window */
    GLFWwindow* _window;
    VkExtent2D _extent{800,600};

    /* Instance/Debug Messenger */
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;

    /* Surface */
    VkSurfaceKHR _surface;

    /* Physical device */
    VkPhysicalDevice _gpu;
    VkPhysicalDeviceProperties _gpuProperties;

    /* Device */
    VkDevice _device;

    /* Swapchain */
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainFormat;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    /* Depth image */
    VkFormat _depthFormat;
    AllocatedImage _depthImage;
    VkImageView _depthImageView;


    /* Queues */
    uint32_t _graphicsQueueFamily;
    VkQueue _graphicsQueue;

    /* Commands */
    //VkCommandPool _commandPool;
    //VkCommandBuffer _commandBuffer;

    /* Render pass */
    VkRenderPass _renderPass;

    /* Framebuffers */
    std::vector<VkFramebuffer> _framebuffers;

    /* Sync */
    //VkFence _renderDone;
    //VkSemaphore _imgReadyFlag;
    //VkSemaphore _renderDoneFlag;

    /* Pipeline */
    //VkPipeline _triPipeline;
    //VkPipelineLayout _triPipelineLayout;


    /* Descriptors */
    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkDescriptorSetLayout _textureSetLayout;
    VkDescriptorPool _descriptorPool;

    VkDescriptorSet _textureSet;

    /* Meshes */
    VkPipelineLayout _meshPipelineLayout;

    VkPipeline _meshPipeline;

    //Mesh _monkeyMesh;
    Mesh _triMesh;

    /* Frames */
    FrameData _frames[NUM_FRAMES];


    /* Uniform buffer */
    //Cpu side data for buffer (for each frame)
    GPUSceneData _sceneParams;
    AllocatedBuffer _sceneParamBuffer;


    /* Upload context for cpu-> gpu asset copying*/
    UploadContext _uploadContext;


    /* Textures */
    VkSampler _blockSampler;
    Texture _mainTexture;
};