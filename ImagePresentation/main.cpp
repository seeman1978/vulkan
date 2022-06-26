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

struct QueueInfo {
    uint32_t FamilyIndex;
    std::vector<float> Priorities;
};

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

    // Load instance level function
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    vkEnumeratePhysicalDevices =
            reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures"));
    PFN_vkCreateDevice vkCreateDevice;
    vkCreateDevice =
            reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(instance, "vkCreateDevice"));
    PFN_vkDestroyInstance vkDestroyInstance;
    vkDestroyInstance =
            reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(instance, "vkDestroyInstance"));
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    vkGetDeviceProcAddr =
            reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));
    // Load physical device extension function
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    for( auto & enabled_extension : desired_extensions ) {
        if( std::string( enabled_extension ) == std::string( VK_KHR_SURFACE_EXTENSION_NAME ) ) {
            vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceSupportKHR" );
            if( vkGetPhysicalDeviceSurfaceSupportKHR == nullptr ) {
                std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceSupportKHR" << std::endl;
                return -1;
            }
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
            if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR == nullptr ) {
                std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceCapabilitiesKHR" << std::endl;
                return -1;
            }
            vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
            if( vkGetPhysicalDeviceSurfaceFormatsKHR == nullptr ) {
                std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceFormatsKHR" << std::endl;
                return -1;
            }
        }
    }

    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    vkEnumerateDeviceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    vkGetPhysicalDeviceQueueFamilyProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));

    //Get physical device
    uint32_t devices_count{0};
    result = vkEnumeratePhysicalDevices(instance, &devices_count, nullptr);
    if( (result != VK_SUCCESS) || (devices_count == 0) ) {
        std::cout << "Could not get the number of available physical devices." << std::endl;
        return -1;
    }
    std::vector<VkPhysicalDevice> physical_devices(devices_count);
    result = vkEnumeratePhysicalDevices(instance, &devices_count, physical_devices.data());
    if( (result != VK_SUCCESS) || (devices_count == 0) ) {
        std::cout << "Could not enumerate physical devices." << std::endl;
        return -1;
    }
    // select a queue family that supports presentation to a given surface
    for (auto physical_device : physical_devices) {
        uint32_t queue_families_count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_families.data());
        //Selecting the index of a queue family with the desired capabilities
        uint32_t queue_family_index, index{};
        bool b_found{false};
        for (auto queue_family : queue_families)  {
            VkBool32 presentation_supported{VK_FALSE};
            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, presentation_surface, &presentation_supported);
            if(result != VK_SUCCESS) {
                std::cout << "Could not vkGetPhysicalDeviceSurfaceSupportKHR." << std::endl;
                return -1;
            }
            if (presentation_supported){
                queue_family_index = index;
                b_found = true;
                break;
            }
            ++index;
        }
        if (!b_found){
            std::cout << "Not found presentation_surface\n";
            continue;
        }

        //Creating a logical device with WSI extensions enabled
        // physical device extension properties
        uint32_t DeviceExtensionProperties_extensions_count = 0;
        result = vkEnumerateDeviceExtensionProperties( physical_device, nullptr,
                                                       &DeviceExtensionProperties_extensions_count, nullptr );
        if( (result != VK_SUCCESS) || (DeviceExtensionProperties_extensions_count == 0) ) {
            std::cout << "Could not get the number of device extensions." << std::endl;
            return -1;
        }
        std::vector<VkExtensionProperties> available_extensions_DeviceExtensionProperties(DeviceExtensionProperties_extensions_count);
        result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &DeviceExtensionProperties_extensions_count, available_extensions_DeviceExtensionProperties.data());
        if( (result != VK_SUCCESS) || (extensions_count == 0) ) {
            std::cout << "Could not enumerate device extensions." << std::endl;
            return -1;
        }
        // physical device desired extensions
        std::vector<char const *> desired_extensions_DeviceExtensionProperties{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        for (auto &extension: desired_extensions_DeviceExtensionProperties) {
            bool b_found{false};
            for (auto available : available_extensions_DeviceExtensionProperties) {
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
        // Creating a logical device with WSI extensions enabled
        std::vector<QueueInfo> queue_infos{{queue_family_index, {1.0f}}};
        // Creating a device queue create info
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (auto queue_info : queue_infos) {
            VkDeviceQueueCreateInfo device_queue;
            device_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            device_queue.pNext = nullptr;
            device_queue.flags = 0;
            device_queue.queueFamilyIndex = queue_info.FamilyIndex;
            device_queue.queueCount = queue_info.Priorities.size();
            device_queue.pQueuePriorities = queue_info.Priorities.data();
            queue_create_infos.emplace_back(device_queue);
        }

        //Getting features and properties of a physical device
        VkPhysicalDeviceFeatures device_features;
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceFeatures(physical_device, &device_features);
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        if (device_features.geometryShader){
            device_features = {};   // keep only geometryShader of device_features
            device_features.geometryShader = VK_TRUE;
        }else{
            continue;
        }

        // Creating a logical device
        VkDeviceCreateInfo device_create_info;
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext = nullptr;
        device_create_info.flags = 0;
        device_create_info.queueCreateInfoCount = queue_create_infos.size();
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.enabledLayerCount = 0;
        device_create_info.ppEnabledLayerNames = nullptr;
        device_create_info.enabledExtensionCount = desired_extensions_DeviceExtensionProperties.size();
        device_create_info.ppEnabledExtensionNames = desired_extensions_DeviceExtensionProperties.empty() ? nullptr : desired_extensions_DeviceExtensionProperties.data();
        device_create_info.pEnabledFeatures = &device_features;
        VkDevice logical_device;
        result = vkCreateDevice(physical_device, &device_create_info, nullptr, &logical_device);
        if( (result != VK_SUCCESS) || (logical_device == VK_NULL_HANDLE) ) {
            std::cout << "Could not create logical device." << std::endl;
            return -1;
        }
        //Selecting a desired presentation mode
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        vkGetPhysicalDeviceSurfacePresentModesKHR =
                reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
        if (vkGetPhysicalDeviceSurfacePresentModesKHR == nullptr){
            std::cout << "Could not load device-level Vulkan function named: vkGetPhysicalDeviceSurfacePresentModesKHR" << std::endl;
            return -1;
        }

        uint32_t present_modes_count;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, presentation_surface, &present_modes_count, nullptr);
        if (result != VK_SUCCESS || present_modes_count==0){
            std::cout << "Could not get the number of supported present modes." <<
                      std::endl;
            return -1;
        }
        std::vector<VkPresentModeKHR> present_modes(present_modes_count);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, presentation_surface, &present_modes_count, present_modes.data());
        if( (VK_SUCCESS != result) ||  (0 == present_modes_count) ) {
            std::cout << "Could not enumerate present modes." << std::endl;
            return -1;
        }
        VkPresentModeKHR desired_present_mode{VK_PRESENT_MODE_MAILBOX_KHR};
        VkPresentModeKHR present_mode;
        b_found = false;
        for (auto current_present_mode : present_modes) {
            if (current_present_mode == desired_present_mode){
                present_mode = desired_present_mode;
                b_found = true;
                break;
            }
        }
        if (!b_found){
            std::cout << "Desired present mode is not supported. Selecting default FIFO mode." << std::endl;
        }
        for (auto current_present_mode : present_modes) {
            if (current_present_mode == VK_PRESENT_MODE_FIFO_KHR){
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                b_found = true;
                break;
            }
        }
        if (!b_found){
            std::cout << "Desired present mode VK_PRESENT_MODE_FIFO_KHR is not supported though it's mandatory for all drivers!" << std::endl;
            return -1;
        }
    }

    return 0;
}
