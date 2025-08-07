module;

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <interrupt.h>
#include <assert.h>

export module bitmap;

export import optional;

export struct bitmap
{

    auto test(size_t bi) -> bool
    {
        auto x = bi / 8;
        auto y = bi % 8;
        return bits[x] >> y & 1;
    }

    // 在位图中 申请连续 cnt 个位 成功则返回 位 下标
    auto scan(size_t cnt) -> optional<size_t>
    {
        size_t i = 0;
        // 跳过满 1 的byte 等于一个小小的剪枝
        while(0xff == bits[i] and i != sz) {
            ++i;
        }
        ASSERT(i != sz);
        if(i == sz) {
            return nullopt;
        }
        // 找到第一个 0 bit的位置
        size_t bi = 0;
        while(bits[i] >> bi & 1) {
            ++bi;
        }
        auto start = i * 8 + bi;
        if(cnt == 1) {
            return start;
        }
        auto left = sz * 8 - start;
        auto it = start + 1;
        auto count = size_t{ 1 };
        while(left--) { // 遍历剩余所有位
            if(not test(it)) {
                ++count;
            } else {
                count = 0;
            }
            if(count == cnt) {
                return it - cnt + 1;
            }
            ++it;
        }
        return nullopt;
    }

    auto set(size_t bi,bool value) -> void
    {
        auto x = bi / 8;
        auto y = bi % 8;
        if(value) {
            bits[x] |= 1 << y;
        } else {
            bits[x] &= ~(1 << y);
        }
    }

    auto set(size_t lbi,size_t rbi,bool value) -> void
    {
        if(rbi - lbi <= 8) {
            for(; lbi != rbi; ++lbi) {
                set(lbi,value);
            }
            return;
        }
        auto l = (lbi + 7) / 8 * 8;
        auto r = rbi / 8 * 8;
        for(; lbi != l; ++lbi) {
            set(lbi,value);
        }
        for(--rbi; rbi >= r; --rbi) {
            set(rbi,value);
        }
        l /= 8;
        r /= 8;
        memset(bits,value ? 0xff : 0,r - l);
    }

    auto clear() const -> void
    { memset(bits,0,sz); }

    u8* bits;
    size_t sz;
};