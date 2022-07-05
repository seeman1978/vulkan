#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstring>


struct WindowParameters{
    xcb_connection_t* connection;
    xcb_screen_t *screen;
    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
};

struct PresentInfo{
    VkSwapchainKHR swapchain;
    uint32_t image_index;
};

struct QueueInfo {
    uint32_t FamilyIndex;
    std::vector<float> Priorities;
};

struct WaitSemaphoreInfo{
    VkSemaphore semaphore;
    VkPipelineStageFlags waitingstage;
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

void init_window(struct WindowParameters &info) {
    uint32_t width{64};
    uint32_t height{64};

    uint32_t value_mask, value_list[32];

    //info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(info.connection, XCB_COPY_FROM_PARENT, info.window, info.screen->root, 0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, info.screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(info.connection, cookie, nullptr);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, nullptr);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE, info.window, (*reply).atom, 4, 32, 1,
                        &(*info.atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(info.connection, info.window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(info.connection, info.window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(info.connection);

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(info.connection))) {
        if ((e->response_type & ~0x80) == XCB_EXPOSE) break;
    }
}

int main() {
    // Load vulkan library
    void *vulkan_library = load_vulkan_library();
    auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    if (vkGetInstanceProcAddr == nullptr){
        std::cout << "Could not find vkGetInstanceProcAddr in Vulkan Runtime library.\n";
        return -1;
    }
    /// Load Global Level Functions
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
        vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
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
    instance_create_info.ppEnabledExtensionNames = desired_extensions.empty() ? nullptr : desired_extensions.data();

    VkInstance instance{};
    if (vkCreateInstance != nullptr){
        VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance);
        if( (result != VK_SUCCESS) || (instance == VK_NULL_HANDLE) ) {
            std::cout << "Could not create Vulkan Instance." << std::endl;
            return -1;
        }
    }

    /// Load instance level function
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
    vkCreateXcbSurfaceKHR =
            reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR"));
    if (vkCreateXcbSurfaceKHR == nullptr){
        std::cout << "Could not load instance level Vulkan function named: vkCreateXcbSurfaceKHR." << std::endl;
        return 1;
    }
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
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    vkGetPhysicalDeviceSurfacePresentModesKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (vkGetPhysicalDeviceSurfacePresentModesKHR == nullptr){
        std::cout << "Could not load device-level Vulkan function named: vkGetPhysicalDeviceSurfacePresentModesKHR" << std::endl;
        return -1;
    }
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    vkDestroySurfaceKHR =
            reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR"));
    if (vkDestroySurfaceKHR == nullptr){
        std::cout << "Could not load device-level Vulkan function named: vkDestroySurfaceKHR" << std::endl;
        return -1;
    }

    // create a presentation surface
    int nScreenNum = 0;
    WindowParameters window_parameters{};
    window_parameters.connection = xcb_connect(nullptr, &nScreenNum);
    if (window_parameters.connection == nullptr || xcb_connection_has_error(window_parameters.connection)) {
        std::cout << "Unable to make an XCB connection\n";
        return -1;
    }
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;

    setup = xcb_get_setup(window_parameters.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (nScreenNum-- > 0){
        xcb_screen_next(&iter);
    }
    window_parameters.screen = iter.data;
    window_parameters.window = xcb_generate_id(window_parameters.connection);
    init_window(window_parameters);

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
        if( queue_families_count == 0 ) {
            std::cout << "Could not get the number of queue families." << std::endl;
            return -1;
        }
        std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_families.data());
        if( queue_families_count == 0 ) {
            std::cout << "Could not acquire properties of queue families." << std::endl;
            return -1;
        }
        //Selecting the index of a queue family with the desired capabilities VK_QUEUE_GRAPHICS_BIT
        bool b_found{false};
        uint32_t GraphicsQueueFamilyIndex;
        for( uint32_t index = 0; index < static_cast<uint32_t>(queue_families.size()); ++index ) {
            if( (queue_families[index].queueCount > 0) && ((queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) ) {
                GraphicsQueueFamilyIndex = index;
                b_found = true;
                break;
            }
        }
        if (!b_found){
            std::cout << "Not found VK_QUEUE_GRAPHICS_BIT\n";
            continue;
        }

        //Selecting the index of a queue family with the desired capabilities PresentationSurface
        uint32_t PresentQueueFamilyIndex;
        b_found = false;
        for( uint32_t index = 0; index < static_cast<uint32_t>(queue_families.size()); ++index ) {
            VkBool32 presentation_supported = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, index, presentation_surface, &presentation_supported );
            if( (VK_SUCCESS == result) && (VK_TRUE == presentation_supported) ) {
                PresentQueueFamilyIndex = index;
                b_found = true;
                break;
            }
        }
        if (!b_found){
            std::cout << "Not found presentation_surface\n";
            continue;
        }

        std::vector<QueueInfo> requested_queues = { { GraphicsQueueFamilyIndex, { 1.0f } } };
        if (GraphicsQueueFamilyIndex != PresentQueueFamilyIndex){
            requested_queues.push_back({ PresentQueueFamilyIndex, { 1.0f } });
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
            bool b_found_extensions{false};
            for (auto available : available_extensions_DeviceExtensionProperties) {
                if (strcmp(available.extensionName, extension) == 0){
                    b_found_extensions = true;
                    break;
                }
            }
            if (!b_found_extensions){
                std::cout << "Extension named '" << extension << "' is not supported."
                          << std::endl;
                return -1;
            }
        }

        // Creating a device queue create info
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (const auto& queue_info : requested_queues) {
            VkDeviceQueueCreateInfo device_queue;
            device_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            device_queue.pNext = nullptr;
            device_queue.flags = 0;
            device_queue.queueFamilyIndex = queue_info.FamilyIndex;
            device_queue.queueCount = queue_info.Priorities.size();
            device_queue.pQueuePriorities = queue_info.Priorities.data();
            queue_create_infos.push_back(device_queue);
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
        // Load device level functions
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
        if( vkAcquireNextImageKHR == nullptr ) {
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
        PFN_vkCreateSemaphore vkCreateSemaphore;
        vkCreateSemaphore =
                reinterpret_cast<PFN_vkCreateSemaphore>(vkGetDeviceProcAddr(logical_device, "vkCreateSemaphore"));
        if( vkCreateSemaphore == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkCreateSemaphore." << std::endl;
            return -1;
        }
        PFN_vkCreateFence vkCreateFence;
        vkCreateFence =
                reinterpret_cast<PFN_vkCreateFence>(vkGetDeviceProcAddr(logical_device, "vkCreateFence"));
        if( vkCreateFence == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkCreateFence." << std::endl;
            return -1;
        }
        PFN_vkCreateCommandPool vkCreateCommandPool;
        vkCreateCommandPool =
                reinterpret_cast<PFN_vkCreateCommandPool>(vkGetDeviceProcAddr(logical_device, "vkCreateCommandPool"));
        if( vkCreateCommandPool == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkCreateCommandPool." << std::endl;
            return -1;
        }
        PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
        vkAllocateCommandBuffers =
                reinterpret_cast<PFN_vkAllocateCommandBuffers>(vkGetDeviceProcAddr(logical_device, "vkAllocateCommandBuffers"));
        if( vkAllocateCommandBuffers == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkAllocateCommandBuffers." << std::endl;
            return -1;
        }
        PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
        vkBeginCommandBuffer =
                reinterpret_cast<PFN_vkBeginCommandBuffer>(vkGetDeviceProcAddr(logical_device, "vkBeginCommandBuffer"));
        if( vkBeginCommandBuffer == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkBeginCommandBuffer." << std::endl;
            return -1;
        }
        PFN_vkEndCommandBuffer vkEndCommandBuffer;
        vkEndCommandBuffer =
                reinterpret_cast<PFN_vkEndCommandBuffer>(vkGetDeviceProcAddr(logical_device, "vkEndCommandBuffer"));
        if( vkEndCommandBuffer == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkEndCommandBuffer." << std::endl;
            return -1;
        }
        PFN_vkResetCommandBuffer vkResetCommandBuffer;
        vkResetCommandBuffer =
                reinterpret_cast<PFN_vkResetCommandBuffer>(vkGetDeviceProcAddr(logical_device, "vkResetCommandBuffer"));
        if( vkResetCommandBuffer == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkResetCommandBuffer." << std::endl;
            return -1;
        }
        PFN_vkResetCommandPool vkResetCommandPool;
        vkResetCommandPool =
                reinterpret_cast<PFN_vkResetCommandPool>(vkGetDeviceProcAddr(logical_device, "vkResetCommandPool"));
        if( vkResetCommandPool == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkResetCommandPool." << std::endl;
            return -1;
        }
        PFN_vkWaitForFences vkWaitForFences;
        vkWaitForFences =
                reinterpret_cast<PFN_vkWaitForFences>(vkGetDeviceProcAddr(logical_device, "vkWaitForFences"));
        if( vkWaitForFences == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkWaitForFences." << std::endl;
            return -1;
        }
        PFN_vkQueueSubmit vkQueueSubmit;
        vkQueueSubmit =
                reinterpret_cast<PFN_vkQueueSubmit>(vkGetDeviceProcAddr(logical_device, "vkQueueSubmit"));
        if( vkQueueSubmit == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkQueueSubmit." << std::endl;
            return -1;
        }
        PFN_vkQueueWaitIdle vkQueueWaitIdle;
        vkQueueWaitIdle =
                reinterpret_cast<PFN_vkQueueWaitIdle>(vkGetDeviceProcAddr(logical_device, "vkQueueWaitIdle"));
        if( vkQueueWaitIdle == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkQueueWaitIdle." << std::endl;
            return -1;
        }
        PFN_vkDestroyFence vkDestroyFence;
        vkDestroyFence =
                reinterpret_cast<PFN_vkDestroyFence>(vkGetDeviceProcAddr(logical_device, "vkDestroyFence"));
        if( vkDestroyFence == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkDestroyFence." << std::endl;
            return -1;
        }
        PFN_vkDestroySemaphore vkDestroySemaphore;
        vkDestroySemaphore =
                reinterpret_cast<PFN_vkDestroySemaphore>(vkGetDeviceProcAddr(logical_device, "vkDestroySemaphore"));
        if( vkDestroySemaphore == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkDestroySemaphore." << std::endl;
            return -1;
        }
        PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
        vkFreeCommandBuffers =
                reinterpret_cast<PFN_vkFreeCommandBuffers>(vkGetDeviceProcAddr(logical_device, "vkFreeCommandBuffers"));
        if( vkFreeCommandBuffers == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkFreeCommandBuffers." << std::endl;
            return -1;
        }
        PFN_vkDestroyCommandPool vkDestroyCommandPool;
        vkDestroyCommandPool =
                reinterpret_cast<PFN_vkDestroyCommandPool>(vkGetDeviceProcAddr(logical_device, "vkDestroyCommandPool"));
        if( vkDestroyCommandPool == nullptr ) {
            std::cout << "Could not load device-level Vulkan function named: vkDestroyCommandPool." << std::endl;
            return -1;
        }

        // Creating a buffer
        VkDeviceSize size{1};
        VkBufferUsageFlags usage{VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        VkBufferCreateInfo buffer_create_info;
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = size;
        buffer_create_info.usage = usage;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VkBuffer buffer;
        result = vkCreateBuffer(logical_device, &buffer_create_info, nullptr, &buffer);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not create a buffer." << std::endl;
            return -1;
        }

        // Get Device Queue
        VkQueue GraphicsQueue;
        vkGetDeviceQueue( logical_device, GraphicsQueueFamilyIndex, 0, &GraphicsQueue );
        VkQueue PresentQueue;
        vkGetDeviceQueue( logical_device, PresentQueueFamilyIndex, 0, &PresentQueue );
        //Selecting a desired presentation mode
        uint32_t present_modes_count{};
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
        // Select present mode
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
        //Getting the capabilities of a presentation surface
        VkSurfaceCapabilitiesKHR surface_capabilities;
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, presentation_surface, &surface_capabilities);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not get the capabilities of a presentation surface." << std::endl;
            return -1;
        }
        //Selecting a number of swapchain images
        uint32_t number_of_images;
        number_of_images = surface_capabilities.minImageCount+1;
        if (surface_capabilities.maxImageCount > 0 && number_of_images > surface_capabilities.maxImageCount){
            number_of_images = surface_capabilities.maxImageCount;
        }
        //Choosing a size of swapchain images
        VkExtent2D size_of_images;
        if (surface_capabilities.currentExtent.width == 0xFFFFFFFF){
            size_of_images.width = 640 < surface_capabilities.minImageExtent.width ? surface_capabilities.minImageExtent.width : 640;
            size_of_images.width = size_of_images.width > surface_capabilities.maxImageExtent.width ? surface_capabilities.maxImageExtent.width : size_of_images.width;
            size_of_images.height = 480;
            if (size_of_images.height < surface_capabilities.minImageExtent.height){
                size_of_images.height = surface_capabilities.minImageExtent.height;
            } else if (size_of_images.height > surface_capabilities.maxImageExtent.height){
                size_of_images.height = surface_capabilities.maxImageExtent.height;
            }
        }else{
            size_of_images = surface_capabilities.currentExtent;
        }

        //Selecting desired usage scenarios of swapchain images
        VkImageUsageFlags desired_usages{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}, image_usage{0};
        image_usage = desired_usages & surface_capabilities.supportedUsageFlags;
        if (desired_usages != image_usage){
            std::cout << "desired_usages is not equal image_usage";
            return -1;
        }
        // Selecting a transformation of swapchain images
        VkSurfaceTransformFlagBitsKHR desired_transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR}, surface_transform;
        if (surface_capabilities.supportedTransforms & desired_transform){
            surface_transform = desired_transform;
        } else{
            surface_transform = surface_capabilities.currentTransform;
        }
        // Selecting a format of swapchain images
        VkSurfaceFormatKHR desired_surface_format{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };  // image format and color-space pair
        uint32_t formats_count;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, presentation_surface, &formats_count, nullptr);
        if (result != VK_SUCCESS || formats_count == 0){
            std::cout << "Could not get the number of supported present formats." << std::endl;
            return -1;
        }
        std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, presentation_surface, &formats_count, surface_formats.data());
        if (result != VK_SUCCESS || formats_count == 0){
            std::cout << "Could not get the number of supported present formats." << std::endl;
            return -1;
        }
        VkFormat image_format;
        VkColorSpaceKHR image_color_space;
        if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED){
            image_format = desired_surface_format.format;
            image_color_space = desired_surface_format.colorSpace;
        }else{
            b_found = false;
            for (auto& surface_format : surface_formats) {
                if (desired_surface_format.format == surface_format.format && desired_surface_format.colorSpace == surface_format.colorSpace){
                    image_format = desired_surface_format.format;
                    image_color_space = desired_surface_format.colorSpace;
                    b_found = true;
                    break;
                }
            }
            if (!b_found){
                for (auto& surface_format : surface_formats) {
                    if (desired_surface_format.format == surface_format.format){
                        image_format = desired_surface_format.format;
                        image_color_space = desired_surface_format.colorSpace;
                        b_found = true;
                        break;
                    }
                }
            }
            if (!b_found){// the format you wanted to use is not supported.
                image_format = surface_formats[0].format;
                image_color_space = surface_formats[0].colorSpace;
                std::cout << "Desired format is not supported. Selecting available format-colorspace combination.\n";
            }
        }
        //Creating a swapchain
        VkSwapchainKHR old_swapchain{VK_NULL_HANDLE};
        VkSwapchainCreateInfoKHR swapchain_create_info;
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.pNext = nullptr;
        swapchain_create_info.flags = 0;
        swapchain_create_info.surface = presentation_surface;
        swapchain_create_info.minImageCount = number_of_images;
        swapchain_create_info.imageFormat = image_format;
        swapchain_create_info.imageColorSpace = image_color_space;
        swapchain_create_info.imageExtent = size_of_images;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = image_usage;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
        swapchain_create_info.preTransform = surface_transform;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = present_mode;
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.oldSwapchain = old_swapchain;
        VkSwapchainKHR swapchain;
        result = vkCreateSwapchainKHR(logical_device, &swapchain_create_info, nullptr, &swapchain);
        if (result != VK_SUCCESS || swapchain == VK_NULL_HANDLE){
            std::cout << "couldn't create a swapchain\n";
            return -1;
        }
        if (old_swapchain != VK_NULL_HANDLE){
            vkDestroySwapchainKHR(logical_device, old_swapchain, nullptr);
            old_swapchain = VK_NULL_HANDLE;
        }
        // Getting handles of swapchain images
        uint32_t images_count;
        result = vkGetSwapchainImagesKHR(logical_device, swapchain, &images_count, nullptr);
        if (result != VK_SUCCESS || images_count == 0){
            std::cout << "could not get the number of swapchain images.\n";
            return -1;
        }
        std::vector<VkImage> swapchain_images(images_count);
        result = vkGetSwapchainImagesKHR(logical_device, swapchain, &images_count, swapchain_images.data());
        if (result != VK_SUCCESS || images_count == 0){
            std::cout << "could not enumerate swapchain images.\n";
            return -1;
        }
        // Acquiring a swapchain image
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo semaphore_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
        result = vkCreateSemaphore(logical_device, &semaphore_create_info, nullptr, &semaphore);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not create a semaphore." << std::endl;
            return -1;
        }
        VkFence fence;
        VkFenceCreateInfo fence_create_info;
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = 0;
        result = vkCreateFence(logical_device, &fence_create_info, nullptr, &fence);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not create a fence." << std::endl;
            return -1;
        }
        uint32_t image_index;
        result = vkAcquireNextImageKHR(logical_device, swapchain, 2000000000, semaphore, fence, &image_index);  // 2000000000 : 2s
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
            std::cout << "could not vkAcquireNextImageKHR swapchain images.\n";
            return -1;
        }

        // Creating a command pool
        VkCommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = nullptr;
        command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_create_info.queueFamilyIndex = GraphicsQueueFamilyIndex;

        VkCommandPool command_pool;
        result = vkCreateCommandPool(logical_device, &command_pool_create_info, nullptr, &command_pool);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not create command pool." << std::endl;
            return -1;
        }
        // Allocating command buffers
        VkCommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = nullptr;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 5;
        std::vector<VkCommandBuffer> command_buffers{5};
        result = vkAllocateCommandBuffers(logical_device, &command_buffer_allocate_info, command_buffers.data());
        if (result != VK_SUCCESS){
            std::cout << "could not allocate command buffers.\n";
            return -1;
        }
        // Beginning a command buffer recording operation
        VkCommandBuffer command_buffer = command_buffers[0];
        VkCommandBufferInheritanceInfo *secondary_command_buffer{nullptr};
        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = secondary_command_buffer;
        result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if (result != VK_SUCCESS){
            std::cout << "Could not begin command buffer recording operation.\n";
            return -1;
        }

        // do something

        // Ending a command buffer recording operation
        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS){
            std::cout << "Error occurred during command buffer recording.\n";
            return -1;
        }

        //Submitting command buffers to a queue
        VkSemaphore rendering_semaphore;
        VkSemaphoreCreateInfo semaphore_create_info2{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
        result = vkCreateSemaphore(logical_device, &semaphore_create_info2, nullptr, &rendering_semaphore);
        if( VK_SUCCESS != result ) {
            std::cout << "Could not create a semaphore." << std::endl;
            return -1;
        }
        std::vector<VkSemaphore> rendering_semaphores{rendering_semaphore};
        WaitSemaphoreInfo wait_semaphore_info{semaphore, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
        std::vector<VkSemaphore> wait_semaphore_handles{wait_semaphore_info.semaphore};
        std::vector<VkPipelineStageFlags> wait_semaphore_stages{wait_semaphore_info.waitingstage};
        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore_handles.size());
        submit_info.pWaitSemaphores = wait_semaphore_handles.data();
        submit_info.pWaitDstStageMask = wait_semaphore_stages.data();
        submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
        submit_info.pCommandBuffers = command_buffers.data();
        submit_info.signalSemaphoreCount = static_cast<uint32_t >(rendering_semaphores.size());
        submit_info.pSignalSemaphores = rendering_semaphores.data();

        result = vkQueueSubmit(PresentQueue, 1, &submit_info, VK_NULL_HANDLE);
        if( VK_SUCCESS != result ) {
            std::cout << "Error occurred during command buffer submission." <<
                      std::endl;
            return -1;
        }

        // Waiting for fences
        std::vector<VkFence> fences{fence};
        VkBool32 wait_for_all{VK_TRUE};
        uint64_t timeout{2000000000};
        result = vkWaitForFences(logical_device, static_cast<uint32_t>(fences.size()), fences.data(), wait_for_all, timeout);
        if (result != VK_SUCCESS){
            std::cout << "Waiting on fence failed.\n";
            return -1;
        }
        // Presenting an image
        std::vector<VkSwapchainKHR> swapchains{swapchain};
        std::vector<uint32_t> image_indices{image_index};
        VkPresentInfoKHR present_info;
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.waitSemaphoreCount = rendering_semaphores.size();
        present_info.pWaitSemaphores = rendering_semaphores.data();
        present_info.swapchainCount = swapchains.size();
        present_info.pSwapchains = swapchains.data();
        present_info.pImageIndices = image_indices.data();
        present_info.pResults = nullptr;
        result = vkQueuePresentKHR(PresentQueue, &present_info);
        if (result != VK_SUCCESS){
            std::cout << "could not vkQueuePresentKHR present images.\n";
            return -1;
        }
        // Waiting until all commands submitted to a queue are finished
        result = vkQueueWaitIdle(GraphicsQueue);
        if (result != VK_SUCCESS){
            std::cout << "Waiting for all operations submitted to queue failed.\n";
            return -1;
        }
        // Waiting for all submitted commands to be finished
        result = vkDeviceWaitIdle(logical_device);
        if (result != VK_SUCCESS){
            std::cout << "Waiting on a device failed.\n";
            return -1;
        }
        // Destroy fence
        if (fence != VK_NULL_HANDLE){
            vkDestroyFence(logical_device, fence, nullptr);
            fence = VK_NULL_HANDLE;
        }

        // Destroy semaphore
        if (semaphore != VK_NULL_HANDLE){
            vkDestroySemaphore(logical_device, semaphore, nullptr);
            semaphore = VK_NULL_HANDLE;
        }
        // Freeing command buffers
        if (!command_buffers.empty()){
            vkFreeCommandBuffers(logical_device, command_pool, command_buffers.size(), command_buffers.data());
            command_buffers.clear();
        }
        // Destroying a command pool
        if (command_pool != VK_NULL_HANDLE){
            vkDestroyCommandPool(logical_device, command_pool, nullptr);
            command_pool = VK_NULL_HANDLE;
        }
        // Destroying a swapchain
        if (swapchain != VK_NULL_HANDLE){
            vkDestroySwapchainKHR(logical_device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }
        // Destroying a presentation surface
        if (presentation_surface != VK_NULL_HANDLE){
            vkDestroySurfaceKHR(instance, presentation_surface, nullptr);
            presentation_surface = VK_NULL_HANDLE;
        }
    }

    return 0;
}
