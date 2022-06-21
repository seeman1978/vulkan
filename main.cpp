#include <iostream>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>

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

struct QueueInfo {
 uint32_t FamilyIndex;
 std::vector<float> Priorities;
};

int main() {
    std::cout << "Hello, World!" << std::endl;
    void *vulkan_library = load_vulkan_library();
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    if (vkGetInstanceProcAddr == nullptr){
        std::cout << "Could not find vkGetInstanceProcAddr in Vulkan Runtime library.\n";
        return -1;
    }
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{};
    PFN_vkEnumerateInstanceLayerProperties  vkEnumerateInstanceLayerProperties;
    PFN_vkCreateInstance vkCreateInstance;
    if (vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceExtensionProperties =
                reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
        vkEnumerateInstanceLayerProperties =
                reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
        vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
    }
    uint32_t extensions_count;
    if (vkEnumerateInstanceExtensionProperties != nullptr){
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
        if( (result != VK_SUCCESS) || (extensions_count == 0)) {
            std::cout << "Could not get the number of Instance extensions." <<
                      std::endl;
            return -1;
        }
        std::vector<VkExtensionProperties> available_extensions(extensions_count);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &available_extensions[0]);
        if( (result != VK_SUCCESS) || (extensions_count == 0) ) {
            std::cout << "Could not enumerate Instance extensions." << std::endl;
            return -1;
        }

        std::vector<char const *> desired_extensions{VK_KHR_SURFACE_EXTENSION_NAME};
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
        result = vkCreateInstance(&instance_create_info, nullptr, &instance);
        if( (result != VK_SUCCESS) || (instance == VK_NULL_HANDLE) ) {
            std::cout << "Could not create Vulkan Instance." << std::endl;
            return -1;
        }

        // instance level
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
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
        vkGetDeviceProcAddr =
                reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));

        // physical device extension
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
        // Enumerate available physical devices
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

        // enumerate physical device
        PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
        vkEnumerateDeviceExtensionProperties =
                reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
        vkGetPhysicalDeviceQueueFamilyProperties =
                reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
        for (auto physical_device : physical_devices) {
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

            // Checking available queue families and their properties
            uint32_t queue_families_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_families.data());

            //Selecting the index of a queue family with
            //the desired capabilities
            uint32_t queue_family_graphics_index{0};
            VkQueueFlags desired_graphics_capabilities{VK_QUEUE_GRAPHICS_BIT};
            for (auto queue_family : queue_families) {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & desired_graphics_capabilities) == desired_graphics_capabilities){
                    break;
                }
                ++queue_family_graphics_index;
            }

            uint32_t queue_family_compute_index{0};
            VkQueueFlags desired_compute_capabilities{VK_QUEUE_COMPUTE_BIT};
            for (auto queue_family : queue_families) {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & desired_compute_capabilities) == desired_compute_capabilities){
                    break;
                }
                ++queue_family_compute_index;
            }

            std::vector<QueueInfo> queue_infos{{queue_family_graphics_index, {1.0f}}};
            if (queue_family_graphics_index != queue_family_compute_index){
                queue_infos.push_back({queue_family_compute_index, {1.0f}});
            }

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
            // Load device level function
            PFN_vkGetDeviceQueue vkGetDeviceQueue;
            vkGetDeviceQueue =
                    reinterpret_cast<PFN_vkGetDeviceQueue>(vkGetDeviceProcAddr(logical_device, "vkGetDeviceQueue"));
            if( vkGetDeviceQueue == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkGetDeviceQueue." << std::endl;
                return -1;
            }
            PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
            vkDeviceWaitIdle =
                    reinterpret_cast<PFN_vkDeviceWaitIdle>(vkGetDeviceProcAddr(logical_device, "vkDeviceWaitIdle"));
            if( vkDeviceWaitIdle == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkDeviceWaitIdle." << std::endl;
                return -1;
            }
            PFN_vkDestroyDevice vkDestroyDevice;
            vkDestroyDevice =
                    reinterpret_cast<PFN_vkDestroyDevice>(vkGetDeviceProcAddr(logical_device, "vkDestroyDevice"));
            if( vkDestroyDevice == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkDestroyDevice." << std::endl;
                return -1;
            }
            PFN_vkCreateBuffer vkCreateBuffer;
            vkCreateBuffer =
                    reinterpret_cast<PFN_vkCreateBuffer>(vkGetDeviceProcAddr(logical_device, "vkCreateBuffer"));
            if( vkCreateBuffer == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkCreateBuffer." << std::endl;
                return -1;
            }
            PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
            vkGetBufferMemoryRequirements =
                    reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(vkGetDeviceProcAddr(logical_device, "vkGetBufferMemoryRequirements"));
            if( vkGetBufferMemoryRequirements == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkGetBufferMemoryRequirements." << std::endl;
                return -1;
            }

            // Load device level function from extension
            PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
            vkCreateSwapchainKHR =
                    reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(logical_device, "vkCreateSwapchainKHR"));
            if( vkCreateSwapchainKHR == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkCreateSwapchainKHR." << std::endl;
                return -1;
            }
            PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
            vkGetSwapchainImagesKHR =
                    reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(logical_device, "vkGetSwapchainImagesKHR"));
            if( vkGetSwapchainImagesKHR == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkGetSwapchainImagesKHR." << std::endl;
                return -1;
            }
            PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
            vkAcquireNextImageKHR =
                    reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(logical_device, "vkAcquireNextImageKHR"));
            if( vkGetSwapchainImagesKHR == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkAcquireNextImageKHR." << std::endl;
                return -1;
            }
            PFN_vkQueuePresentKHR vkQueuePresentKHR;
            vkQueuePresentKHR =
                    reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(logical_device, "vkQueuePresentKHR"));
            if( vkQueuePresentKHR == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkQueuePresentKHR." << std::endl;
                return -1;
            }
            PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
            vkDestroySwapchainKHR =
                    reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(logical_device, "vkDestroySwapchainKHR"));
            if( vkDestroySwapchainKHR == nullptr ) {
                std::cout << "Could not load device-level Vulkan function named: vkDestroySwapchainKHR." << std::endl;
                return -1;
            }

            // create a logical device with geometry shaders, graphics and compute queues
            VkQueue graphics_queue, compute_queue;
            vkGetDeviceQueue(logical_device, queue_family_graphics_index, 0, &graphics_queue);
            vkGetDeviceQueue(logical_device, queue_family_compute_index, 0, &compute_queue);
            return 0;
        }
    }

    return 0;
}
