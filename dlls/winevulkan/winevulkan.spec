# Automatically generated from Vulkan vk.xml; DO NOT EDIT!
#
# This file is generated from Vulkan vk.xml file covered
# by the following copyright and permission notice:
#
# Copyright (c) 2015-2020 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ---- Exceptions to the Apache 2.0 License: ----
#
# As an exception, if you use this Software to generate code and portions of
# this Software are embedded into the generated code as a result, you may
# redistribute such product without providing attribution as would otherwise
# be required by Sections 4(a), 4(b) and 4(d) of the License.
#
# In addition, if you combine or link code generated by this Software with
# software that is licensed under the GPLv2 or the LGPL v2.0 or 2.1
# ("`Combined Software`") and if a court of competent jurisdiction determines
# that the patent provision (Section 3), the indemnity provision (Section 9)
# or other Section of the License conflicts with the conditions of the
# applicable GPL or LGPL license, you may retroactively and prospectively
# choose to deem waived or otherwise exclude such Section(s) of the License,
# but only in their entirety and only with respect to the Combined Software.
#

@ stdcall -private vk_icdGetInstanceProcAddr(ptr str) wine_vk_icdGetInstanceProcAddr
@ stdcall -private vk_icdNegotiateLoaderICDInterfaceVersion(ptr) wine_vk_icdNegotiateLoaderICDInterfaceVersion
@ cdecl -norelay native_vkGetInstanceProcAddrWINE(ptr str)
@ stdcall -private wine_vkAcquireNextImage2KHR(ptr ptr ptr)
@ stdcall -private wine_vkAcquireNextImageKHR(ptr int64 int64 int64 int64 ptr)
@ stdcall -private wine_vkAllocateCommandBuffers(ptr ptr ptr)
@ stdcall -private wine_vkAllocateDescriptorSets(ptr ptr ptr)
@ stdcall -private wine_vkAllocateMemory(ptr ptr ptr ptr)
@ stdcall -private wine_vkBeginCommandBuffer(ptr ptr)
@ stdcall -private wine_vkBindBufferMemory(ptr int64 int64 int64)
@ stdcall -private wine_vkBindBufferMemory2(ptr long ptr)
@ stdcall -private wine_vkBindImageMemory(ptr int64 int64 int64)
@ stdcall -private wine_vkBindImageMemory2(ptr long ptr)
@ stdcall -private wine_vkCmdBeginQuery(ptr int64 long long)
@ stdcall -private wine_vkCmdBeginRenderPass(ptr ptr long)
@ stdcall -private wine_vkCmdBeginRenderPass2(ptr ptr ptr)
@ stdcall -private wine_vkCmdBindDescriptorSets(ptr long int64 long long ptr long ptr)
@ stdcall -private wine_vkCmdBindIndexBuffer(ptr int64 int64 long)
@ stdcall -private wine_vkCmdBindPipeline(ptr long int64)
@ stdcall -private wine_vkCmdBindVertexBuffers(ptr long long ptr ptr)
@ stdcall -private wine_vkCmdBlitImage(ptr int64 long int64 long long ptr long)
@ stdcall -private wine_vkCmdClearAttachments(ptr long ptr long ptr)
@ stdcall -private wine_vkCmdClearColorImage(ptr int64 long ptr long ptr)
@ stdcall -private wine_vkCmdClearDepthStencilImage(ptr int64 long ptr long ptr)
@ stdcall -private wine_vkCmdCopyBuffer(ptr int64 int64 long ptr)
@ stdcall -private wine_vkCmdCopyBufferToImage(ptr int64 int64 long long ptr)
@ stdcall -private wine_vkCmdCopyImage(ptr int64 long int64 long long ptr)
@ stdcall -private wine_vkCmdCopyImageToBuffer(ptr int64 long int64 long ptr)
@ stdcall -private wine_vkCmdCopyQueryPoolResults(ptr int64 long long int64 int64 int64 long)
@ stdcall -private wine_vkCmdDispatch(ptr long long long)
@ stdcall -private wine_vkCmdDispatchBase(ptr long long long long long long)
@ stdcall -private wine_vkCmdDispatchIndirect(ptr int64 int64)
@ stdcall -private wine_vkCmdDraw(ptr long long long long)
@ stdcall -private wine_vkCmdDrawIndexed(ptr long long long long long)
@ stdcall -private wine_vkCmdDrawIndexedIndirect(ptr int64 int64 long long)
@ stdcall -private wine_vkCmdDrawIndexedIndirectCount(ptr int64 int64 int64 int64 long long)
@ stdcall -private wine_vkCmdDrawIndirect(ptr int64 int64 long long)
@ stdcall -private wine_vkCmdDrawIndirectCount(ptr int64 int64 int64 int64 long long)
@ stdcall -private wine_vkCmdEndQuery(ptr int64 long)
@ stdcall -private wine_vkCmdEndRenderPass(ptr)
@ stdcall -private wine_vkCmdEndRenderPass2(ptr ptr)
@ stdcall -private wine_vkCmdExecuteCommands(ptr long ptr)
@ stdcall -private wine_vkCmdFillBuffer(ptr int64 int64 int64 long)
@ stdcall -private wine_vkCmdNextSubpass(ptr long)
@ stdcall -private wine_vkCmdNextSubpass2(ptr ptr ptr)
@ stdcall -private wine_vkCmdPipelineBarrier(ptr long long long long ptr long ptr long ptr)
@ stdcall -private wine_vkCmdPushConstants(ptr int64 long long long ptr)
@ stdcall -private wine_vkCmdResetEvent(ptr int64 long)
@ stdcall -private wine_vkCmdResetQueryPool(ptr int64 long long)
@ stdcall -private wine_vkCmdResolveImage(ptr int64 long int64 long long ptr)
@ stdcall -private wine_vkCmdSetBlendConstants(ptr ptr)
@ stdcall -private wine_vkCmdSetDepthBias(ptr float float float)
@ stdcall -private wine_vkCmdSetDepthBounds(ptr float float)
@ stdcall -private wine_vkCmdSetDeviceMask(ptr long)
@ stdcall -private wine_vkCmdSetEvent(ptr int64 long)
@ stdcall -private wine_vkCmdSetLineWidth(ptr float)
@ stdcall -private wine_vkCmdSetScissor(ptr long long ptr)
@ stdcall -private wine_vkCmdSetStencilCompareMask(ptr long long)
@ stdcall -private wine_vkCmdSetStencilReference(ptr long long)
@ stdcall -private wine_vkCmdSetStencilWriteMask(ptr long long)
@ stdcall -private wine_vkCmdSetViewport(ptr long long ptr)
@ stdcall -private wine_vkCmdUpdateBuffer(ptr int64 int64 int64 ptr)
@ stdcall -private wine_vkCmdWaitEvents(ptr long ptr long long long ptr long ptr long ptr)
@ stdcall -private wine_vkCmdWriteTimestamp(ptr long int64 long)
@ stdcall -private wine_vkCreateBuffer(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateBufferView(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateCommandPool(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateComputePipelines(ptr int64 long ptr ptr ptr)
@ stdcall -private wine_vkCreateDescriptorPool(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateDescriptorSetLayout(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateDescriptorUpdateTemplate(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateDevice(ptr ptr ptr ptr)
@ stub vkCreateDisplayModeKHR
@ stub vkCreateDisplayPlaneSurfaceKHR
@ stdcall -private wine_vkCreateEvent(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateFence(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateFramebuffer(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateGraphicsPipelines(ptr int64 long ptr ptr ptr)
@ stdcall -private wine_vkCreateImage(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateImageView(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateInstance(ptr ptr ptr)
@ stdcall -private wine_vkCreatePipelineCache(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreatePipelineLayout(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateQueryPool(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateRenderPass(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateRenderPass2(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateSampler(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateSamplerYcbcrConversion(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateSemaphore(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateShaderModule(ptr ptr ptr ptr)
@ stub vkCreateSharedSwapchainsKHR
@ stdcall -private wine_vkCreateSwapchainKHR(ptr ptr ptr ptr)
@ stdcall -private wine_vkCreateWin32SurfaceKHR(ptr ptr ptr ptr)
@ stdcall -private wine_vkDestroyBuffer(ptr int64 ptr)
@ stdcall -private wine_vkDestroyBufferView(ptr int64 ptr)
@ stdcall -private wine_vkDestroyCommandPool(ptr int64 ptr)
@ stdcall -private wine_vkDestroyDescriptorPool(ptr int64 ptr)
@ stdcall -private wine_vkDestroyDescriptorSetLayout(ptr int64 ptr)
@ stdcall -private wine_vkDestroyDescriptorUpdateTemplate(ptr int64 ptr)
@ stdcall -private wine_vkDestroyDevice(ptr ptr)
@ stdcall -private wine_vkDestroyEvent(ptr int64 ptr)
@ stdcall -private wine_vkDestroyFence(ptr int64 ptr)
@ stdcall -private wine_vkDestroyFramebuffer(ptr int64 ptr)
@ stdcall -private wine_vkDestroyImage(ptr int64 ptr)
@ stdcall -private wine_vkDestroyImageView(ptr int64 ptr)
@ stdcall -private wine_vkDestroyInstance(ptr ptr)
@ stdcall -private wine_vkDestroyPipeline(ptr int64 ptr)
@ stdcall -private wine_vkDestroyPipelineCache(ptr int64 ptr)
@ stdcall -private wine_vkDestroyPipelineLayout(ptr int64 ptr)
@ stdcall -private wine_vkDestroyQueryPool(ptr int64 ptr)
@ stdcall -private wine_vkDestroyRenderPass(ptr int64 ptr)
@ stdcall -private wine_vkDestroySampler(ptr int64 ptr)
@ stdcall -private wine_vkDestroySamplerYcbcrConversion(ptr int64 ptr)
@ stdcall -private wine_vkDestroySemaphore(ptr int64 ptr)
@ stdcall -private wine_vkDestroyShaderModule(ptr int64 ptr)
@ stdcall -private wine_vkDestroySurfaceKHR(ptr int64 ptr)
@ stdcall -private wine_vkDestroySwapchainKHR(ptr int64 ptr)
@ stdcall -private wine_vkDeviceWaitIdle(ptr)
@ stdcall -private wine_vkEndCommandBuffer(ptr)
@ stdcall -private wine_vkEnumerateDeviceExtensionProperties(ptr str ptr ptr)
@ stdcall -private wine_vkEnumerateDeviceLayerProperties(ptr ptr ptr)
@ stdcall -private wine_vkEnumerateInstanceExtensionProperties(str ptr ptr)
@ stdcall -private wine_vkEnumerateInstanceLayerProperties(ptr ptr)
@ stdcall -private wine_vkEnumerateInstanceVersion(ptr)
@ stdcall -private wine_vkEnumeratePhysicalDeviceGroups(ptr ptr ptr)
@ stdcall -private wine_vkEnumeratePhysicalDevices(ptr ptr ptr)
@ stdcall -private wine_vkFlushMappedMemoryRanges(ptr long ptr)
@ stdcall -private wine_vkFreeCommandBuffers(ptr int64 long ptr)
@ stdcall -private wine_vkFreeDescriptorSets(ptr int64 long ptr)
@ stdcall -private wine_vkFreeMemory(ptr int64 ptr)
@ stdcall -private wine_vkGetBufferDeviceAddress(ptr ptr)
@ stdcall -private wine_vkGetBufferMemoryRequirements(ptr int64 ptr)
@ stdcall -private wine_vkGetBufferMemoryRequirements2(ptr ptr ptr)
@ stdcall -private wine_vkGetBufferOpaqueCaptureAddress(ptr ptr)
@ stdcall -private wine_vkGetDescriptorSetLayoutSupport(ptr ptr ptr)
@ stdcall -private wine_vkGetDeviceGroupPeerMemoryFeatures(ptr long long long ptr)
@ stdcall -private wine_vkGetDeviceGroupPresentCapabilitiesKHR(ptr ptr)
@ stdcall -private wine_vkGetDeviceGroupSurfacePresentModesKHR(ptr int64 ptr)
@ stdcall -private wine_vkGetDeviceMemoryCommitment(ptr int64 ptr)
@ stdcall -private wine_vkGetDeviceMemoryOpaqueCaptureAddress(ptr ptr)
@ stdcall -private wine_vkGetDeviceProcAddr(ptr str)
@ stdcall -private wine_vkGetDeviceQueue(ptr long long ptr)
@ stdcall -private wine_vkGetDeviceQueue2(ptr ptr ptr)
@ stub vkGetDisplayModePropertiesKHR
@ stub vkGetDisplayPlaneCapabilitiesKHR
@ stub vkGetDisplayPlaneSupportedDisplaysKHR
@ stdcall -private wine_vkGetEventStatus(ptr int64)
@ stdcall -private wine_vkGetFenceStatus(ptr int64)
@ stdcall -private wine_vkGetImageMemoryRequirements(ptr int64 ptr)
@ stdcall -private wine_vkGetImageMemoryRequirements2(ptr ptr ptr)
@ stdcall -private wine_vkGetImageSparseMemoryRequirements(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetImageSparseMemoryRequirements2(ptr ptr ptr ptr)
@ stdcall -private wine_vkGetImageSubresourceLayout(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetInstanceProcAddr(ptr str)
@ stub vkGetPhysicalDeviceDisplayPlanePropertiesKHR
@ stub vkGetPhysicalDeviceDisplayPropertiesKHR
@ stdcall -private wine_vkGetPhysicalDeviceExternalBufferProperties(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceExternalFenceProperties(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceExternalSemaphoreProperties(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceFeatures(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceFeatures2(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceFormatProperties(ptr long ptr)
@ stdcall -private wine_vkGetPhysicalDeviceFormatProperties2(ptr long ptr)
@ stdcall -private wine_vkGetPhysicalDeviceImageFormatProperties(ptr long long long long long ptr)
@ stdcall -private wine_vkGetPhysicalDeviceImageFormatProperties2(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceMemoryProperties(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceMemoryProperties2(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDevicePresentRectanglesKHR(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceProperties(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceProperties2(ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceQueueFamilyProperties(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceQueueFamilyProperties2(ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSparseImageFormatProperties(ptr long long long long long ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSparseImageFormatProperties2(ptr ptr ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ptr int64 ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSurfaceFormatsKHR(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSurfacePresentModesKHR(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetPhysicalDeviceSurfaceSupportKHR(ptr long int64 ptr)
@ stdcall -private wine_vkGetPhysicalDeviceWin32PresentationSupportKHR(ptr long)
@ stdcall -private wine_vkGetPipelineCacheData(ptr int64 ptr ptr)
@ stdcall -private wine_vkGetQueryPoolResults(ptr int64 long long long ptr int64 long)
@ stdcall -private wine_vkGetRenderAreaGranularity(ptr int64 ptr)
@ stdcall -private wine_vkGetSemaphoreCounterValue(ptr int64 ptr)
@ stdcall -private wine_vkGetSwapchainImagesKHR(ptr int64 ptr ptr)
@ stdcall -private wine_vkInvalidateMappedMemoryRanges(ptr long ptr)
@ stdcall -private wine_vkMapMemory(ptr int64 int64 int64 long ptr)
@ stdcall -private wine_vkMergePipelineCaches(ptr int64 long ptr)
@ stdcall -private wine_vkQueueBindSparse(ptr long ptr int64)
@ stdcall -private wine_vkQueuePresentKHR(ptr ptr)
@ stdcall -private wine_vkQueueSubmit(ptr long ptr int64)
@ stdcall -private wine_vkQueueWaitIdle(ptr)
@ stdcall -private wine_vkResetCommandBuffer(ptr long)
@ stdcall -private wine_vkResetCommandPool(ptr int64 long)
@ stdcall -private wine_vkResetDescriptorPool(ptr int64 long)
@ stdcall -private wine_vkResetEvent(ptr int64)
@ stdcall -private wine_vkResetFences(ptr long ptr)
@ stdcall -private wine_vkResetQueryPool(ptr int64 long long)
@ stdcall -private wine_vkSetEvent(ptr int64)
@ stdcall -private wine_vkSignalSemaphore(ptr ptr)
@ stdcall -private wine_vkTrimCommandPool(ptr int64 long)
@ stdcall -private wine_vkUnmapMemory(ptr int64)
@ stdcall -private wine_vkUpdateDescriptorSetWithTemplate(ptr int64 int64 ptr)
@ stdcall -private wine_vkUpdateDescriptorSets(ptr long ptr long ptr)
@ stdcall -private wine_vkWaitForFences(ptr long ptr long int64)
@ stdcall -private wine_vkWaitSemaphores(ptr ptr int64)
