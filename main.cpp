#include <iostream>
#include <dlfcn.h>
#include <vulkan/vulkan.h>

void *load_vulkan_library(){
    void *vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW);
    if (vulkan_library == nullptr){
        std::cout << "Could not connect with a Vulkan Runtime library." <<
                  std::endl;
        return nullptr;
    }
    std::cout << "Connect with a Vulkan Runtime library successfully." <<
              std::endl;
    return vulkan_library;
}

// create global level function
void create_global_level_func(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr){
    if (vkGetInstanceProcAddr != nullptr){
        PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties =
                reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
        PFN_vkEnumerateInstanceLayerProperties  vkEnumerateInstanceLayerProperties =
                reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
        PFN_vkCreateInstance vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    void *vulkan_library = load_vulkan_library();
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    if (vkGetInstanceProcAddr == nullptr){
        std::cout << "Could not find vkGetInstanceProcAddr in Vulkan Runtime library.\n";
        return -1;
    }
    create_global_level_func(vkGetInstanceProcAddr);
    return 0;
}
