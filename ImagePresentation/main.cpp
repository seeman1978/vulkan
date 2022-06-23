#include <iostream>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>

struct WindowParameters{
    xcb_connection_t*             connection;
    xcb_window_t                  window;
};

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
            return -1;
        }
    }
    PFN_vkEnumerateInstanceLayerProperties  vkEnumerateInstanceLayerProperties;
    if (vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceLayerProperties =
                reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
        if (vkEnumerateInstanceLayerProperties == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkEnumerateInstanceLayerProperties." << std::endl;
            return 1;
        }
    }
    PFN_vkCreateInstance vkCreateInstance;
    if (vkGetInstanceProcAddr != nullptr){
        vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (vkCreateInstance == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkCreateInstance." << std::endl;
            return -1;
        }
    }
    // Get available_extensions
    uint32_t extensions_count{};
    if (vkEnumerateInstanceExtensionProperties != nullptr) {
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
        if( (result != VK_SUCCESS) || (extensions_count == 0)) {
            std::cout << "Could not get the number of Instance extensions." <<
                      std::endl;
            return -1;
        }
    }
    std::vector<VkExtensionProperties> available_extensions(extensions_count);
    if (vkEnumerateInstanceExtensionProperties != nullptr) {
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, available_extensions.data());
        if ((result != VK_SUCCESS) || (extensions_count == 0)) {
            std::cout << "Could not enumerate Instance extensions." << std::endl;
            return -1;
        }
    }

    // Get desired_extensions
    std::vector<char const *> desired_extensions{VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
    for (auto &extension: desired_extensions) {
        bool b_found{false};
        for (auto available : available_extensions) {
            if (strcmp(available.extensionName, extension) == 0){
                b_found = true;
                break;
            }
        }
        if (!b_found){
            std::cout << "Extension named '" << extension << "' is not supported."
                      << std::endl;
            return -1;
        }
    }
    // create a vulkan instance with WSI extensions enabled
    VkApplicationInfo application_info;
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = nullptr;
    application_info.pApplicationName = "WQ";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 2, 3);
    application_info.pEngineName = "First";
    application_info.engineVersion = VK_MAKE_VERSION(2, 3, 4);
    application_info.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );

    VkInstanceCreateInfo instance_create_info;
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.ppEnabledLayerNames = nullptr;
    instance_create_info.enabledExtensionCount = desired_extensions.size();
    instance_create_info.ppEnabledExtensionNames = desired_extensions.empty() ? nullptr : &desired_extensions[0];

    VkInstance instance;
    if (vkCreateInstance != nullptr){
        VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance);
        if( (result != VK_SUCCESS) || (instance == VK_NULL_HANDLE) ) {
            std::cout << "Could not create Vulkan Instance." << std::endl;
            return -1;
        }
    }

    // Load instance level function
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
    vkCreateXcbSurfaceKHR =
            reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
    if (vkCreateXcbSurfaceKHR == nullptr){
        std::cout << "Could not load instance level Vulkan function named: vkCreateXcbSurfaceKHR." << std::endl;
        return 1;
    }

    // create a presentation surface
    int nScreenNum = 0;
    WindowParameters window_parameters;
    window_parameters.connection = xcb_connect("vulkan", &nScreenNum);
    window_parameters.window = xcb_generate_id(window_parameters.connection);

    VkXcbSurfaceCreateInfoKHR surface_create_info;
    surface_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.connection = window_parameters.connection;
    surface_create_info.window = window_parameters.window;

    VkSurfaceKHR presentation_surface{VK_NULL_HANDLE};
    VkResult result = vkCreateXcbSurfaceKHR(instance, &surface_create_info, nullptr, &presentation_surface);
    if( (VK_SUCCESS != result) || (VK_NULL_HANDLE == presentation_surface) ) {
        std::cout << "Could not create presentation surface." << std::endl;
        return -1;
    }
    return 0;
}
