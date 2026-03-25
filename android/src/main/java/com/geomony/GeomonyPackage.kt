package com.geomony

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider

class GeomonyPackage : BaseReactPackage() {
    override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? {
        return if (name == GeomonyModule.NAME) {
            GeomonyModule(reactContext)
        } else {
            null
        }
    }

    override fun getReactModuleInfoProvider() = ReactModuleInfoProvider {
        mapOf(
            GeomonyModule.NAME to ReactModuleInfo(
                name = GeomonyModule.NAME,
                className = GeomonyModule.NAME,
                canOverrideExistingModule = false,
                needsEagerInit = false,
                isCxxModule = false,
                isTurboModule = true
            )
        )
    }
}
