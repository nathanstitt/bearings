require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "Geomony"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/nathanstitt/geomony.git", :tag => "#{s.version}" }

  s.source_files = "ios/**/*.{h,m,mm,swift,cpp}", "cpp/src/**/*.cpp"
  s.private_header_files = "ios/**/*.h", "cpp/include/**/*.h"
  s.frameworks   = "CoreLocation", "CoreMotion"
  s.libraries    = "sqlite3"

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/cpp/include" "$(PODS_TARGET_SRCROOT)/cpp/vendor"',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17'
  }

  install_modules_dependencies(s)
end
