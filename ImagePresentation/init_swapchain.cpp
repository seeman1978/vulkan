//
// Created by wq on 22-6-28.
//
/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Inititalize Swapchain
*/

/* This is part of the draw cube progression */

//#include "util_init.hpp"
#include <cassert>
#include <cstdlib>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include "util.hpp"

void *g_vulkan_library;
PFN_vkGetInstanceProcAddr g_vkGetInstanceProcAddr;

void *load_vulkan_library(){
    void *vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW);
    if (vulkan_library == nullptr){
        std::cout << "Could not connect with a Vulkan Runtime library." << std::endl;
        return nullptr;
    }
    std::cout << "Connect with a Vulkan Runtime library successfully." <<
              std::endl;
    return vulkan_library;
}

/*
 * TODO: function description here
 */
VkResult init_global_extension_properties(layer_properties &layer_props) {
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{};
    if (g_vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceExtensionProperties =
                reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(g_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
        if (vkEnumerateInstanceExtensionProperties == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkEnumerateInstanceExtensionProperties." << std::endl;
            return VK_ERROR_UNKNOWN;
        }
    }

    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = nullptr;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res) return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.instance_extensions.resize(instance_extension_count);
        instance_extensions = layer_props.instance_extensions.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

VkResult init_global_layer_properties(struct sample_info &info) {
    PFN_vkEnumerateInstanceLayerProperties  vkEnumerateInstanceLayerProperties;
    if (g_vkGetInstanceProcAddr != nullptr){
        vkEnumerateInstanceLayerProperties =
                reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >(g_vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
        if (vkEnumerateInstanceLayerProperties == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkEnumerateInstanceLayerProperties." << std::endl;
            return VK_ERROR_UNKNOWN;
        }
    }

    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = nullptr;
    VkResult res;
#ifdef __ANDROID__
    // This place is the first place for samples to use Vulkan APIs.
    // Here, we are going to open Vulkan.so on the device and retrieve function pointers using
    // vulkan_wrapper helper.
    if (!InitVulkan()) {
        LOGE("Failied initializing Vulkan APIs!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    LOGI("Loaded Vulkan APIs.");
#endif

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
        if (res) return res;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        res = init_global_extension_properties(layer_props);
        if (res) return res;
        info.instance_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    return res;
}


void init_instance_extension_names(struct sample_info &info) {
    info.instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef __ANDROID__
    info.instance_extension_names.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_WIN32)
    info.instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    info.instance_extension_names.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    info.instance_extension_names.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
    info.instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
}

void init_device_extension_names(struct sample_info &info) {
    info.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}


VkResult init_instance(struct sample_info &info, char const *const app_short_name) {
    PFN_vkCreateInstance vkCreateInstance;
    if (g_vkGetInstanceProcAddr != nullptr){
        vkCreateInstance =
                reinterpret_cast<PFN_vkCreateInstance>(g_vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
        if (vkCreateInstance == nullptr){
            std::cout << "Could not load global level Vulkan function named: vkCreateInstance." << std::endl;
            return VK_ERROR_UNKNOWN;
        }
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = app_short_name;
    app_info.applicationVersion = 1;
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = info.instance_layer_names.size();
    inst_info.ppEnabledLayerNames = info.instance_layer_names.size() ? info.instance_layer_names.data() : NULL;
    inst_info.enabledExtensionCount = info.instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = info.instance_extension_names.data();

    VkResult res = vkCreateInstance(&inst_info, NULL, &info.inst);
    assert(res == VK_SUCCESS);

    return res;
}


VkResult init_device_extension_properties(struct sample_info &info, layer_properties &layer_props) {
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    vkEnumerateDeviceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(g_vkGetInstanceProcAddr(info.inst, "vkEnumerateDeviceExtensionProperties"));

    VkExtensionProperties *device_extensions;
    uint32_t device_extension_count;
    VkResult res;
    char *layer_name = nullptr;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateDeviceExtensionProperties(info.gpus[0], layer_name, &device_extension_count, NULL);
        if (res) return res;

        if (device_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.device_extensions.resize(device_extension_count);
        device_extensions = layer_props.device_extensions.data();
        res = vkEnumerateDeviceExtensionProperties(info.gpus[0], layer_name, &device_extension_count, device_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

VkResult init_enumerate_device(struct sample_info &info, uint32_t gpu_count) {
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    vkEnumeratePhysicalDevices =
            reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(g_vkGetInstanceProcAddr(info.inst, "vkEnumeratePhysicalDevices"));

    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    vkGetPhysicalDeviceQueueFamilyProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(g_vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceQueueFamilyProperties"));
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(g_vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceMemoryProperties"));
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(g_vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceProperties"));

    uint32_t const U_ASSERT_ONLY req_count = gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, nullptr);
    assert(gpu_count);
    info.gpus.resize(gpu_count);

    res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, info.gpus.data());
    assert(!res && gpu_count >= req_count);

    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_family_count, nullptr);
    assert(info.queue_family_count >= 1);

    info.queue_props.resize(info.queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_family_count, info.queue_props.data());
    assert(info.queue_family_count >= 1);

    /* This is as good a place as any to do this */
    vkGetPhysicalDeviceMemoryProperties(info.gpus[0], &info.memory_properties);
    vkGetPhysicalDeviceProperties(info.gpus[0], &info.gpu_props);
    /* query device extensions for enabled layers */
    for (auto &layer_props : info.instance_layer_properties) {
        init_device_extension_properties(info, layer_props);
    }

    return res;
}


void init_window_size(struct sample_info &info, int32_t default_width, int32_t default_height) {
#ifdef __ANDROID__
    AndroidGetWindowSize(&info.width, &info.height);
#else
    info.width = default_width;
    info.height = default_height;
#endif
}

void init_connection(struct sample_info &info) {
#if defined(VK_USE_PLATFORM_XCB_KHR)
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    info.connection = xcb_connect(NULL, &scr);
    if (info.connection == NULL || xcb_connection_has_error(info.connection)) {
        std::cout << "Unable to make an XCB connection\n";
        exit(-1);
    }

    setup = xcb_get_setup(info.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);

    info.screen = iter.data;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    info.display = wl_display_connect(nullptr);

    if (info.display == nullptr) {
        printf(
            "Cannot find a compatible Vulkan installable client driver "
            "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    info.registry = wl_display_get_registry(info.display);
    wl_registry_add_listener(info.registry, &registry_listener, &info);
    wl_display_dispatch(info.display);
#endif
}

void init_window(struct sample_info &info) {
    assert(info.width > 0);
    assert(info.height > 0);

    uint32_t value_mask, value_list[32];

    info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(info.connection, XCB_COPY_FROM_PARENT, info.window, info.screen->root, 0, 0, info.width, info.height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, info.screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, 0);

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

VkResult init_device(struct sample_info &info) {
    PFN_vkCreateDevice vkCreateDevice;
    vkCreateDevice =
            reinterpret_cast<PFN_vkCreateDevice>(g_vkGetInstanceProcAddr(info.inst, "vkCreateDevice"));

    VkResult res;
    VkDeviceQueueCreateInfo queue_info = {};

    float queue_priorities[1] = {0.0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;
    queue_info.queueFamilyIndex = info.graphics_queue_family_index;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? info.device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = NULL;

    res = vkCreateDevice(info.gpus[0], &device_info, NULL, &info.device);
    assert(res == VK_SUCCESS);

    return res;
}

int main(int argc, char *argv[]) {
    // Load vulkan library
    void* vulkan_library = load_vulkan_library();
    g_vulkan_library = vulkan_library;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vulkan_library, "vkGetInstanceProcAddr"));
    g_vkGetInstanceProcAddr = vkGetInstanceProcAddr;
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

    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Swapchain Initialization Sample";

    /*
     * Set up swapchain:
     * - Get supported uses for all queues
     * - Try to find a queue that supports both graphics and present
     * - If no queue supports both, find a present queue and make sure we have a
     *   graphics queue
     * - Get a list of supported formats and use the first one
     * - Get surface properties and present modes and use them to create a swap
     *   chain
     * - Create swap chain buffers
     * - For each buffer, create a color attachment view and set its layout to
     *   color attachment
     */

    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info, 1);
    init_window_size(info, 64, 64);
    init_connection(info);
    init_window(info);

    // Load instance level function
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
    vkCreateXcbSurfaceKHR =
            reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(info.inst, "vkCreateXcbSurfaceKHR"));
    if (vkCreateXcbSurfaceKHR == nullptr){
        std::cout << "Could not load instance level Vulkan function named: vkCreateXcbSurfaceKHR." << std::endl;
        return 1;
    }

    // Load physical device extension function
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;

    vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr( info.inst, "vkGetPhysicalDeviceSurfaceSupportKHR" );
    if( vkGetPhysicalDeviceSurfaceSupportKHR == nullptr ) {
        std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceSupportKHR" << std::endl;
        return -1;
    }
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr( info.inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
    if( vkGetPhysicalDeviceSurfaceCapabilitiesKHR == nullptr ) {
        std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceCapabilitiesKHR" << std::endl;
        return -1;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr( info.inst, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
    if( vkGetPhysicalDeviceSurfaceFormatsKHR == nullptr ) {
        std::cout << "Could not load instance-level Vulkan function named: vkGetPhysicalDeviceSurfaceFormatsKHR" << std::endl;
        return -1;
    }

    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    vkEnumerateDeviceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(vkGetInstanceProcAddr(info.inst, "vkEnumerateDeviceExtensionProperties"));
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    vkGetPhysicalDeviceQueueFamilyProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceQueueFamilyProperties"));
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    vkGetPhysicalDeviceSurfacePresentModesKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (vkGetPhysicalDeviceSurfacePresentModesKHR == nullptr){
        std::cout << "Could not load device-level Vulkan function named: vkGetPhysicalDeviceSurfacePresentModesKHR" << std::endl;
        return -1;
    }

/* VULKAN_KEY_START */
// Construct the surface description:
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.hinstance = info.connection;
    createInfo.hwnd = info.window;
    res = vkCreateWin32SurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#elif defined(__ANDROID__)
    GET_INSTANCE_PROC_ADDR(info.inst, CreateAndroidSurfaceKHR);

    VkAndroidSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = AndroidGetApplicationWindow();
    res = info.fpCreateAndroidSurfaceKHR(info.inst, &createInfo, nullptr, &info.surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VkWaylandSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.display = info.display;
    createInfo.surface = info.window;
    res = vkCreateWaylandSurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#else
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.connection = info.connection;
    createInfo.window = info.window;
    res = vkCreateXcbSurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#endif  // _WIN32
    assert(res == VK_SUCCESS);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *pSupportsPresent = (VkBool32 *)malloc(info.queue_family_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < info.queue_family_count; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(info.gpus[0], i, info.surface, &pSupportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    info.graphics_queue_family_index = UINT32_MAX;
    info.present_queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < info.queue_family_count; ++i) {
        if ((info.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (info.graphics_queue_family_index == UINT32_MAX) info.graphics_queue_family_index = i;

            if (pSupportsPresent[i] == VK_TRUE) {
                info.graphics_queue_family_index = i;
                info.present_queue_family_index = i;
                break;
            }
        }
    }

    if (info.present_queue_family_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (size_t i = 0; i < info.queue_family_count; ++i)
            if (pSupportsPresent[i] == VK_TRUE) {
                info.present_queue_family_index = i;
                break;
            }
    }
    free(pSupportsPresent);

    // Generate error if could not find queues that support graphics
    // and present
    if (info.graphics_queue_family_index == UINT32_MAX || info.present_queue_family_index == UINT32_MAX) {
        std::cout << "Could not find a queues for graphics and "
                     "present\n";
        exit(-1);
    }

    init_device(info);

    // Get the list of VkFormats that are supported:
    uint32_t formatCount;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(info.gpus[0], info.surface, &formatCount, NULL);
    assert(res == VK_SUCCESS);
    VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(info.gpus[0], info.surface, &formatCount, surfFormats);
    assert(res == VK_SUCCESS);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        info.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        assert(formatCount >= 1);
        info.format = surfFormats[0].format;
    }
    free(surfFormats);

    VkSurfaceCapabilitiesKHR surfCapabilities;

    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info.gpus[0], info.surface, &surfCapabilities);
    assert(res == VK_SUCCESS);

    uint32_t presentModeCount;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(info.gpus[0], info.surface, &presentModeCount, NULL);
    assert(res == VK_SUCCESS);
    VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));

    res = vkGetPhysicalDeviceSurfacePresentModesKHR(info.gpus[0], info.surface, &presentModeCount, presentModes);
    assert(res == VK_SUCCESS);

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = info.width;
        swapchainExtent.height = info.height;
        if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
            swapchainExtent.width = surfCapabilities.minImageExtent.width;
        } else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width) {
            swapchainExtent.width = surfCapabilities.maxImageExtent.width;
        }

        if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
            swapchainExtent.height = surfCapabilities.minImageExtent.height;
        } else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height) {
            swapchainExtent.height = surfCapabilities.maxImageExtent.height;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount;

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); i++) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = NULL;
    swapchain_ci.surface = info.surface;
    swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
    swapchain_ci.imageFormat = info.format;
    swapchain_ci.imageExtent.width = swapchainExtent.width;
    swapchain_ci.imageExtent.height = swapchainExtent.height;
    swapchain_ci.preTransform = preTransform;
    swapchain_ci.compositeAlpha = compositeAlpha;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.presentMode = swapchainPresentMode;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
    swapchain_ci.clipped = true;
    swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = NULL;
    uint32_t queueFamilyIndices[2] = {(uint32_t)info.graphics_queue_family_index, (uint32_t)info.present_queue_family_index};
    if (info.graphics_queue_family_index != info.present_queue_family_index) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between
        // the queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
    }

    // Load instance level function
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    vkEnumeratePhysicalDevices =
            reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(info.inst, "vkEnumeratePhysicalDevices"));
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceProperties"));
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(vkGetInstanceProcAddr(info.inst, "vkGetPhysicalDeviceFeatures"));
    PFN_vkCreateDevice vkCreateDevice;
    vkCreateDevice =
            reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(info.inst, "vkCreateDevice"));
    PFN_vkDestroyInstance vkDestroyInstance;
    vkDestroyInstance =
            reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(info.inst, "vkDestroyInstance"));
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    vkGetDeviceProcAddr =
            reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetInstanceProcAddr(info.inst, "vkGetDeviceProcAddr"));

    // Load device level function from extension
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    vkCreateSwapchainKHR =
            reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(info.device, "vkCreateSwapchainKHR"));
    if( vkCreateSwapchainKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkCreateSwapchainKHR." << std::endl;
        return -1;
    }
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    vkGetSwapchainImagesKHR =
            reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(info.device, "vkGetSwapchainImagesKHR"));
    if( vkGetSwapchainImagesKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkGetSwapchainImagesKHR." << std::endl;
        return -1;
    }
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    vkAcquireNextImageKHR =
            reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(info.device, "vkAcquireNextImageKHR"));
    if( vkGetSwapchainImagesKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkAcquireNextImageKHR." << std::endl;
        return -1;
    }
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    vkQueuePresentKHR =
            reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(info.device, "vkQueuePresentKHR"));
    if( vkQueuePresentKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkQueuePresentKHR." << std::endl;
        return -1;
    }
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    vkDestroySwapchainKHR =
            reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(info.device, "vkDestroySwapchainKHR"));
    if( vkDestroySwapchainKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkDestroySwapchainKHR." << std::endl;
        return -1;
    }
    PFN_vkCreateImageView vkCreateImageView;
    vkCreateImageView =
            reinterpret_cast<PFN_vkCreateImageView>(vkGetDeviceProcAddr(info.device, "vkCreateImageView"));
    if( vkCreateImageView == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkCreateImageView." << std::endl;
        return -1;
    }
    PFN_vkDestroyImageView vkDestroyImageView;
    vkDestroyImageView =
            reinterpret_cast<PFN_vkDestroyImageView>(vkGetDeviceProcAddr(info.device, "vkDestroyImageView"));
    if( vkDestroyImageView == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkDestroyImageView." << std::endl;
        return -1;
    }
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    vkDestroySurfaceKHR =
            reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetDeviceProcAddr(info.device, "vkDestroySurfaceKHR"));
    if( vkDestroySurfaceKHR == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkDestroySurfaceKHR." << std::endl;
        return -1;
    }
    PFN_vkDestroyDevice vkDestroyDevice;
    vkDestroyDevice =
            reinterpret_cast<PFN_vkDestroyDevice>(vkGetDeviceProcAddr(info.device, "vkDestroyDevice"));
    if( vkDestroyDevice == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkDestroyDevice." << std::endl;
        return -1;
    }
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
    vkDeviceWaitIdle =
            reinterpret_cast<PFN_vkDeviceWaitIdle>(vkGetDeviceProcAddr(info.device, "vkDeviceWaitIdle"));
    if( vkDeviceWaitIdle == nullptr ) {
        std::cout << "Could not load device-level Vulkan function named: vkDeviceWaitIdle." << std::endl;
        return -1;
    }

    res = vkCreateSwapchainKHR(info.device, &swapchain_ci, NULL, &info.swap_chain);
    assert(res == VK_SUCCESS);

    res = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, NULL);
    assert(res == VK_SUCCESS);

    VkImage *swapchainImages = (VkImage *)malloc(info.swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    res = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, swapchainImages);
    assert(res == VK_SUCCESS);

    info.buffers.resize(info.swapchainImageCount);
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        info.buffers[i].image = swapchainImages[i];
    }
    free(swapchainImages);

    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.flags = 0;
        color_image_view.image = info.buffers[i].image;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.format = info.format;
        color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;

        res = vkCreateImageView(info.device, &color_image_view, nullptr, &info.buffers[i].view);
        assert(res == VK_SUCCESS);
    }

    /* VULKAN_KEY_END */

    /* Clean Up */
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyImageView(info.device, info.buffers[i].view, nullptr);
    }
    vkDestroySwapchainKHR(info.device, info.swap_chain, nullptr);

    vkDeviceWaitIdle(info.device);
    vkDestroyDevice(info.device, nullptr);

    vkDestroySurfaceKHR(info.inst, info.surface, nullptr);
    xcb_destroy_window(info.connection, info.window);
    xcb_disconnect(info.connection);

    vkDestroyInstance(info.inst, nullptr);

    return 0;
}
