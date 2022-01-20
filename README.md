Build steps for jsenv
===========================
GN (in short of 'Generate Ninja') is a meta-build system that generates Ninja
build files so that you can build projects with Ninja.
Public GN documents can be found at: [https://chromium.googlesource.com/chromium/src/+/master/tools/gn/README.md](https://chromium.googlesource.com/chromium/src/+/master/tools/gn/README.md)

## 1. Pull docker image and enter container.
```
  docker pull quickapp/v8-build:200513
```

## 2. Get lfs file.
v8-build-0.0.5.tgz is a v8 static library which compiled from the
v8:8.3-lkgr-SNAPSHOT of V8 with v8-build-patch.
For more information about patch, you can view v8-build-patch directory. 
```
git clone [jsenv-runtime.git]
git lfs install
git lfs pull
tar -xf v8-build-0.0.5.tgz
```

## 3. Link third_party.
ln -s /root/v8/third_party/android_ndk third_party/android_tools/ndk
ln -s /root/v8/third_party/android_sdk/public third_party/android_tools/sdk
ln -s /root/v8/third_party/depot_tools third_party/depot_tools

## 4. Build jsenv.

* Only build jsenv.
```
  python3 build/build_upload.py --test --version [jsenv-version]
```

* Build and upload jsenv.
```
  python3 build/build_upload.py --upload --test \
    --version [jsenv-version] --repo-username admin --repo-password [password]
```
