#include "matching_engine.hpp"

#include <jni.h>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {

matching::MatchingEngine* engineFrom(const jlong handle) {
    if (handle == 0) {
        throw std::invalid_argument("native engine is closed");
    }
    return reinterpret_cast<matching::MatchingEngine*>(handle);
}

void throwJava(JNIEnv* env, const char* className, const char* message) {
    const jclass exceptionClass = env->FindClass(className);
    if (exceptionClass != nullptr) {
        env->ThrowNew(exceptionClass, message);
    }
}

} // namespace

extern "C" {

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

        std::vector<jlong> flattened;
        flattened.reserve(trades.size() * 5);
        for (const auto& trade : trades) {
            flattened.push_back(trade.tradeId);
            flattened.push_back(trade.makerOrderId);
            flattened.push_back(trade.takerOrderId);
            flattened.push_back(trade.price);
            flattened.push_back(trade.quantity);
        }

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

} // extern "C"
