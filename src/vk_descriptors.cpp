#include <vk_descriptors.h>

#include <algorithm>
#include <set>
#include <set>

#include "fmt/ranges.h"

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBinding = {};
    newBinding.binding = binding;
    newBinding.descriptorType = type;
    newBinding.descriptorCount = 1;

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& binding : bindings)
    {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

/*
The 3 parameters in the ImageInfo can be optional, depending on the specific VkDescriptorType.

VK_DESCRIPTOR_TYPE_SAMPLER: JUST the sampler, so it does not need ImageView or layout to be set.
VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: doesn't need the sampler set because its going to be accessed with different
                                  samplers within the shader, this descriptor type is just a pointer to the image.
VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: needs everything set, as it holds the information for both the sampler,
                                        and the image it samples. This is a useful type because it means we only
                                        need 1 descriptor binding to access the texture.
VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: was used back in chapter 2, it does not need sampler, and it's used to allow
                                  compute shaders to directly access pixel data.
*/
void DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout,
    VkDescriptorType type)
{
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo
    {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo
    {
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize
        {
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
    // Destroy all the descriptors created from the pool and put it back to initialize state.
    // Won't delete VkDescriptorPool itself.
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

    return ds;
}

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios)
{
    ratios.clear();

    for (auto ratio : poolRatios)
    {
        ratios.push_back(ratio);
    }

    VkDescriptorPool newPool = create_pool(device, initialSets, poolRatios);
    setsPerPool = initialSets * 1.5f; //  grow it next allocation
    readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
{
    for (auto pool : readyPools)
    {
        vkResetDescriptorPool(device, pool, 0);
    }
    for (auto pool : fullPools)
    {
        vkResetDescriptorPool(device, pool, 0);
        readyPools.push_back(pool);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
{
    for (auto pool : readyPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    for (auto pool : fullPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    //get or create a pool to allocate from
    VkDescriptorPool poolToUse = get_pool(device);

    VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;

    // Allocation failed. Try again
    if (VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
        result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        fullPools.push_back(poolToUse);
    
        poolToUse = get_pool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK( vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }
  
    readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (!readyPools.empty())
    {
        newPool = readyPools.back();
        readyPools.pop_back();
    }
    else
    {
        // Need to create a new pool
        newPool = create_pool(device, setsPerPool, ratios);
        setsPerPool = setsPerPool * 1.5f;
        setsPerPool = std::min<uint32_t>(setsPerPool, 4092);
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t setCount,
    std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize
        {
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = setCount;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
    return newPool;
}
