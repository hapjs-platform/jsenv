Build steps for jsenv
===========================
GN (in short of 'Generate Ninja') is a meta-build system that generates Ninja
build files so that you can build projects with Ninja.
Public GN documents can be found at: [https://chromium.googlesource.com/chromium/src/+/master/tools/gn/README.md](https://chromium.googlesource.com/chromium/src/+/master/tools/gn/README.md)

## 1. Pull docker image and enter container.
```
  docker pull quickapp/v8-build:200513
```

## 2. build v8
```
export CI_PROJECT_DIR="/root/jsenv-runtime"
cd /root/v8
git checkout remotes/origin/8.3-lkgr
cp -R /root/v8/include $CI_PROJECT_DIR/v8-build/v8
cd $CI_PROJECT_DIR/v8-build/v8/src
python build_v8.py --target android --v8-source /root/v8 --remote-branch remotes/origin/8.3-lkgr
```
## 3. Link third_party.
```
cd $CI_PROJECT_DIR
mkdir -p third_party/android_tools/
ln -s /root/v8/third_party/android_ndk third_party/android_tools/ndk
ln -s /root/v8/third_party/android_sdk/public third_party/android_tools/sdk
ln -s /root/v8/third_party/depot_tools third_party/depot_tools
```

## 4. Build jsenv.

* Only build jsenv.
```
  python3 build_upload.py --test --version [jsenv-version]
```

* Build and upload jsenv.
```
  python3 build_upload.py --upload --test \
    --version [jsenv-version] --repo-username admin --repo-password [password]
```
