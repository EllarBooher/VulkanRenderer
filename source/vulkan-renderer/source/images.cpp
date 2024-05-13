#include "images.hpp"

#include "helpers.hpp"
#include "initializers.hpp"

VkOffset3D ExtentToOffset(VkExtent3D extent)
{
    auto const x = static_cast<int32_t>(extent.width);
    auto const y = static_cast<int32_t>(extent.height);
    auto const z = static_cast<int32_t>(extent.depth);

    return { x, y, z };
}

void vkutil::transitionImage(
    VkCommandBuffer const cmd
    , VkImage const image
    , VkImageLayout const oldLayout
    , VkImageLayout const newLayout
    , VkImageAspectFlags const aspects
)
{
    VkImageMemoryBarrier2 const imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,

        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

        .oldLayout = oldLayout,
        .newLayout = newLayout,

        .image = image,
        .subresourceRange = vkinit::imageSubresourceRange(aspects),
    };

    VkDependencyInfo const depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vkutil::recordCopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent3D dstSize)
{
    VkImageBlit2 const blitRegion{
        .sType{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 },
        .pNext{ nullptr },
        .srcSubresource{ vkinit::imageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0) },
        .srcOffsets{
            VkOffset3D{},
            ExtentToOffset(srcSize),
        },
        .dstSubresource{ vkinit::imageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0) },
        .dstOffsets{
            VkOffset3D{},
            ExtentToOffset(dstSize),
        },
    };

    VkBlitImageInfo2 const blitInfo{
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext{ nullptr },
        .srcImage{ source },
        .srcImageLayout{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
        .dstImage{ destination },
        .dstImageLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
        .regionCount{ 1 },
        .pRegions{ &blitRegion },
        .filter{ VK_FILTER_LINEAR }
    };

    vkCmdBlitImage2(cmd, &blitInfo);
}

std::optional<AllocatedImage> AllocatedImage::allocate(
    VmaAllocator allocator
    , VkDevice device
    , VkExtent3D extent
    , VkFormat format
    , VkImageAspectFlags viewFlags
    , VkImageUsageFlags usageMask
)
{
    AllocatedImage image{
        .allocation{ VK_NULL_HANDLE },
        .image{ VK_NULL_HANDLE },
        .imageView{ VK_NULL_HANDLE },

        .imageExtent = extent,
        .imageFormat = format
    };

    VkImageCreateInfo const imageInfo{ 
        vkinit::imageCreateInfo(
            image.imageFormat
            , VK_IMAGE_LAYOUT_UNDEFINED
            , usageMask
            , image.imageExtent
        ) 
    };

    VmaAllocationCreateInfo const imageAllocInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    VkResult const createImageResult{
        vmaCreateImage(allocator, &imageInfo, &imageAllocInfo, &image.image, &image.allocation, nullptr)
    };
    if (createImageResult != VK_SUCCESS)
    {
        LogVkResult(createImageResult, "VMA Allocation failed");
        return {};
    }

    VkImageViewCreateInfo const imageViewInfo{
        vkinit::imageViewCreateInfo(
            image.imageFormat,
            image.image,
            viewFlags
        )
    };

    VkResult const createViewResult{
        vkCreateImageView(device, &imageViewInfo, nullptr, &image.imageView)
    };
    if (createViewResult != VK_SUCCESS)
    {
        LogVkResult(createViewResult, "vkCreateImageView failed");
        return {};
    }

    return image;
}