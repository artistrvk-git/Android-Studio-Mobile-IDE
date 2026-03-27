# Android-Studio-Mobile-IDE

# Android Studio Mobile 🚀
**Presented By RVK EDITION** | Version 1.0.0

Android Studio Mobile ek powerful Android IDE environment hai jisko mobile devices ke liye design kiya gaya hai. Ye core application class pure file system, SDKs, aur background services ko efficiently initialize karti hai.

## ✨ Core Features
* **Multi-Environment Support:** Android SDK, JDK (17 & 21), NDK, Gradle, Flutter, NodeJS, aur Python ke liye dedicated isolated directories set up karta hai.
* **Native Engine Integration:** High-performance compiler aur IDE tasks handle karne ke liye `rvk_engine` C/C++ library load karta hai.
* **Smart Notification Channels:** Build progress, heavy SDK downloads, aur background services ko track karne ke liye Android O+ devices par custom notification channels create karta hai.
* **Auto Directory Initialization:** App launch hote hi saare zaroori folders external storage mein automatically create ho jate hain.

## 📁 Directory Structure
App aapke phone ke external storage mein ek base folder set karta hai: `Current Storage/AndroidStudioMobile`. Iske andar ye essential directories maintain hoti hain:

* `/Projects` - Aapke saare app projects yahan save hote hain.
* `/SDK` - Android SDK files aur platform tools.
* `/JDK` - Java Development Kits (Support for jdk-17 & jdk-21).
* `/NDK` - Native Development Kit for C/C++ support.
* `/Gradle` - Gradle build system files (Default v8.14.4).
* `/Flutter`, `/NodeJS`, `/Python` - Cross-platform aur scripting languages ke liye environment folders.
* `/Logs`, `/.temp`, `/.cache` - App logs, temporary build files, aur cache memory manage karne ke liye.

## 🛠️ System Channels Setup
App background tasks ko in channels ke through manage karta hai:
* **Build Progress (`rvk_build_channel`):** Shows real-time build results aur errors.
* **SDK Downloads (`rvk_download_channel`):** SDKs aur external tools ki downloading progress track karta hai.
* **Background Services (`rvk_service_channel`):** Background mein chalne wale long IDE tasks ke liye.

## 🚀 Getting Started
Kyunki ye app ki base `Application` class hai, make sure aapne isko apne `AndroidManifest.xml` mein properly register kiya hua hai:

```xml
<application
    android:name=".RVKApplication"
    android:requestLegacyExternalStorage="true"
    ... >
    </application>
