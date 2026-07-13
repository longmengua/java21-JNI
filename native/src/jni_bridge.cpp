#include "matching_engine.hpp"

#include <jni.h>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {

// 把 Java 傳進來的 handle 還原成 C++ 物件指標。
matching::MatchingEngine* engineFrom(const jlong handle) {
    if (handle == 0) {
        throw std::invalid_argument("native engine is closed");
    }
    return reinterpret_cast<matching::MatchingEngine*>(handle);
}

// 將 C++ 例外轉成 Java 例外。
void throwJava(JNIEnv* env, const char* className, const char* message) {
    const jclass exceptionClass = env->FindClass(className);
    if (exceptionClass != nullptr) {
        env->ThrowNew(exceptionClass, message);
    }
}

} // namespace

extern "C" {

// 建立新的 C++ MatchingEngine，回傳給 Java 當作 handle。
JNIEXPORT jlong JNICALL
Java_com_example_demo_matching_NativeMatchingEngine_nativeCreate(
    JNIEnv* env,
    jclass
) {
    try {
        return reinterpret_cast<jlong>(new matching::MatchingEngine());
    } catch (const std::exception& exception) {
        throwJava(env, "java/lang/IllegalStateException", exception.what());
        return 0;
    }
}

// Java 關閉時呼叫，釋放 C++ 物件。
JNIEXPORT void JNICALL
Java_com_example_demo_matching_NativeMatchingEngine_nativeDestroy(
    JNIEnv* env,
    jclass,
    const jlong handle
) {
    try {
        delete engineFrom(handle);
    } catch (const std::exception& exception) {
        throwJava(env, "java/lang/IllegalStateException", exception.what());
    }
}

// 送單並回傳成交結果。
// 回傳格式是扁平化陣列：
// [tradeId, makerOrderId, takerOrderId, price, quantity, ...]
JNIEXPORT jlongArray JNICALL
Java_com_example_demo_matching_NativeMatchingEngine_nativeSubmit(
    JNIEnv* env,
    jclass,
    const jlong handle,
    const jlong orderId,
    const jint side,
    const jlong price,
    const jlong quantity
) {
    try {
        if (side != 0 && side != 1) {
            throw std::invalid_argument("side must be BUY or SELL");
        }

        const auto trades = engineFrom(handle)->submit(
            static_cast<std::int64_t>(orderId),
            static_cast<matching::Side>(side),
            static_cast<std::int64_t>(price),
            static_cast<std::int64_t>(quantity)
        );

        // 每筆成交 5 個欄位，所以先預留 trades.size() * 5。
        std::vector<jlong> flattened;
        flattened.reserve(trades.size() * 5);
        for (const auto& trade : trades) {
            flattened.push_back(trade.tradeId);
            flattened.push_back(trade.makerOrderId);
            flattened.push_back(trade.takerOrderId);
            flattened.push_back(trade.price);
            flattened.push_back(trade.quantity);
        }

        // 先建立 Java long[]，再把 C++ 內容拷貝回去。
        const auto output = env->NewLongArray(static_cast<jsize>(flattened.size()));
        if (output == nullptr) {
            return nullptr;
        }
        if (!flattened.empty()) {
            env->SetLongArrayRegion(
                output,
                0,
                static_cast<jsize>(flattened.size()),
                flattened.data()
            );
        }
        return output;
    } catch (const std::invalid_argument& exception) {
        throwJava(env, "java/lang/IllegalArgumentException", exception.what());
        return nullptr;
    } catch (const std::exception& exception) {
        throwJava(env, "java/lang/IllegalStateException", exception.what());
        return nullptr;
    }
}

// 撤單。
JNIEXPORT jboolean JNICALL
Java_com_example_demo_matching_NativeMatchingEngine_nativeCancel(
    JNIEnv* env,
    jclass,
    const jlong handle,
    const jlong orderId
) {
    try {
        return engineFrom(handle)->cancel(orderId) ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& exception) {
        throwJava(env, "java/lang/IllegalStateException", exception.what());
        return JNI_FALSE;
    }
}

// 查詢目前所有掛單。
// 回傳格式是扁平化陣列：
// [orderId, side, price, quantity, ...]
JNIEXPORT jlongArray JNICALL
Java_com_example_demo_matching_NativeMatchingEngine_nativeListOrders(
    JNIEnv* env,
    jclass,
    const jlong handle
) {
    try {
        const auto orders = engineFrom(handle)->listOrders();

        // 每筆掛單 4 個欄位。
        std::vector<jlong> flattened;
        flattened.reserve(orders.size() * 4);
        for (const auto& order : orders) {
            flattened.push_back(order.orderId);
            flattened.push_back(static_cast<jlong>(order.side == matching::Side::Buy ? 0 : 1));
            flattened.push_back(order.price);
            flattened.push_back(order.quantity);
        }

        // 先建立 Java long[]，再把 C++ 資料拷貝回去。
        const auto output = env->NewLongArray(static_cast<jsize>(flattened.size()));
        if (output == nullptr) {
            return nullptr;
        }
        if (!flattened.empty()) {
            env->SetLongArrayRegion(
                output,
                0,
                static_cast<jsize>(flattened.size()),
                flattened.data()
            );
        }
        return output;
    } catch (const std::exception& exception) {
        throwJava(env, "java/lang/IllegalStateException", exception.what());
        return nullptr;
    }
}

} // extern "C"
