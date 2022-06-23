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

int main() {
    // Load vulkan library
    void *vulkan_library = load_vulkan_library();
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    if (vkGetInstanceProcAddr == nullptr){
        std::cout << "Could not find vkGetInstanceProcAddr in Vulkan Runtime library.\n";
        return -1;
    }
    // Load Global Level Functions
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{};
    if (vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceExtensionProperties =
                reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
        if (vkEnumerateInstanceExtensionProperties == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkEnumerateInstanceExtensionProperties." << std::endl;
        }
    }
    PFN_vkEnumerateInstanceLayerProperties  vkEnumerateInstanceLayerProperties;
    if (vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceLayerProperties =
                reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
        if (vkEnumerateInstanceLayerProperties == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkEnumerateInstanceLayerProperties." << std::endl;
        }
    }
    PFN_vkCreateInstance vkCreateInstance;
    if (vkGetInstanceProcAddr != nullptr){
        vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (vkCreateInstance == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkCreateInstance." << std::endl;
        }
    }
    // create a vulkan instance with WSI extensions enabled

    return 0;
}
