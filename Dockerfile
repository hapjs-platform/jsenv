FROM ubuntu:19.10

ENV PATH=${PATH}:/root/depot_tools:/root/v8/tools/dev

# for tz config, see https://serverfault.com/questions/683605/docker-container-time-timezone-will-not-reflect-changes
ENV TZ=America/Los_Angeles
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN mkdir -p ~/.config/fish && echo 'alias gm=~/v8/tools/dev/gm.py' >> ~/.config/fish/config.fish

RUN apt update -qq && apt upgrade -y && apt-get install -qq -y --no-install-recommends \
        ca-certificates \
        gnupg2 \
        python \
        sudo \
        lsb-core \
        vim \
        fish \
        curl \
        git

RUN apt-get install -y npm && \
      npm i -g n && \
      npm i -g yarn && \
      n latest

RUN cd ~ && git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git --depth=1

RUN cd ~ && fetch v8 && cd ~/v8 && gclient sync
RUN apt install gcc-9-arm-linux-gnueabihf -y

RUN cd ~/v8 && sed -i 's/${dev_list} snapcraft/${dev_list}/g' build/install-build-deps.sh && build/install-build-deps.sh --lib32

CMD [ "fish" ]

ENV ANDROID_HOME=/root/v8/third_party/android_sdk/public
ENV ANDROID_NDK_HOME=/root/v8/third_party/android_ndk

RUN apt install openjdk-8-jdk -y && update-java-alternatives -s java-1.8.0-openjdk-amd64

RUN cd ~/v8 && build/install-build-deps-android.sh
RUN cd ~/v8/build && git reset HEAD --hard

RUN cd ~/v8 && echo "target_os = ['android']" >> ../.gclient \
        && gclient sync


RUN apt update && apt install libatomic1:i386 -y

ENV PATH=${PATH}:${ANDROID_HOME}/cmdline-tools/latest/bin
ARG ANDROID_BUILD_VERSION=28
ARG ANDROID_TOOLS_VERSION=28.0.3
RUN yes | sdkmanager --licenses \
        && yes | sdkmanager "platforms;android-$ANDROID_BUILD_VERSION" \
        "build-tools;$ANDROID_TOOLS_VERSION"
