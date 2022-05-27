/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2019 JUUL Labs
 * Copyright (c) 2020 Arm Limited
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

#include <stddef.h>

#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "bootutil_priv.h"

/*
 * Initialize a TLV iterator.
 *
 * @param it An iterator struct
 * @param hdr image_header of the slot's image
 * @param fap flash_area of the slot which is storing the image
 * @param type Type of TLV to look for
 * @param prot true if TLV has to be stored in the protected area, false otherwise
 *
 * @returns 0 if the TLV iterator was successfully started
 *          -1 on errors
 */
/* 初始化 tlv 迭代器 */
int
bootutil_tlv_iter_begin(struct image_tlv_iter *it, const struct image_header *hdr,
                        const struct flash_area *fap, uint16_t type, bool prot)
{
    uint32_t off_;
    struct image_tlv_info info;

    if (it == NULL || hdr == NULL || fap == NULL) {
        return -1;
    }

    /* tlvs 紧紧跟在 image 后 */
    off_ = BOOT_TLV_OFF(hdr);
    /* 读取 image_tlv_info 可以认为是 tlv 的 header 信息 */
    if (LOAD_IMAGE_DATA(hdr, fap, off_, &info, sizeof(info))) {
        return -1;
    }

    /* 确认 tlv 的类型，是否是 PROTECTED */
    if (info.it_magic == IMAGE_TLV_PROT_INFO_MAGIC) {
        if (hdr->ih_protect_tlv_size != info.it_tlv_tot) {
            return -1;
        }

        /* 读取下一组 tlv */
        if (LOAD_IMAGE_DATA(hdr, fap, off_ + info.it_tlv_tot,
                            &info, sizeof(info))) {
            return -1;
        }
    } else if (hdr->ih_protect_tlv_size != 0) {
        return -1;
    }

    /* 确认是否是普通类型的 tlv */
    if (info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        return -1;
    }

    /* 初始化 tlv 迭代器 */
    it->hdr = hdr;
    it->fap = fap;
    it->type = type;
    it->prot = prot;
    it->prot_end = off_ + it->hdr->ih_protect_tlv_size;
    it->tlv_end = off_ + it->hdr->ih_protect_tlv_size + info.it_tlv_tot;
    // position on first TLV
    // 第一个 tlv 的偏移信息(可能是 protected 也可能是一般的 tlv)
    it->tlv_off = off_ + sizeof(info);
    return 0;
}

/*
 * Find next TLV
 *
 * @param it The image TLV iterator struct
 * @param off The offset of the TLV's payload in flash
 * @param len The length of the TLV's payload
 * @param type If not NULL returns the type of TLV found
 *
 * @returns 0 if a TLV with with matching type was found
 *          1 if no more TLVs with matching type are available
 *          -1 on errors
 */
int
bootutil_tlv_iter_next(struct image_tlv_iter *it, uint32_t *off, uint16_t *len,
                       uint16_t *type)
{
    struct image_tlv tlv;
    int rc;

    if (it == NULL || it->hdr == NULL || it->fap == NULL) {
        return -1;
    }

    while (it->tlv_off < it->tlv_end) {
        /* 如果遍历到了 protected 类型的 tlv 尾部，那么需要你加上一般的 tlv
         * 头部的偏移，偏移量是 struct image_tlv_info 结构体大小
         * */
        if (it->hdr->ih_protect_tlv_size > 0 && it->tlv_off == it->prot_end) {
            it->tlv_off += sizeof(struct image_tlv_info);
        }

        /* 读取这个 tlv 信息 */
        rc = LOAD_IMAGE_DATA(it->hdr, it->fap, it->tlv_off, &tlv, sizeof tlv);
        if (rc) {
            return -1;
        }

        /* No more TLVs in the protected area */
        /* 如果指定遍历 protected tlv 并且检测到遍历的 tlv 超出了 protected area 区域
         * 那么就返回 1
         * */
        if (it->prot && it->tlv_off >= it->prot_end) {
            return 1;
        }

        if (it->type == IMAGE_TLV_ANY || tlv.it_type == it->type) {
            if (type != NULL) {
                *type = tlv.it_type;
            }
            *off = it->tlv_off + sizeof(tlv);
            *len = tlv.it_len;
            it->tlv_off += sizeof(tlv) + tlv.it_len;
            return 0;
        }

        it->tlv_off += sizeof(tlv) + tlv.it_len;
    }

    return 1;
}
