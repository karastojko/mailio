
# mailio #

*mailio* 是一个用于构建MIME格式和实现SMTP以及POP3协议的跨平台C++库，基于C++17标准，使用Boost。

# 用例 #

为了发送一封邮件，你必须创建一个 `message` 对象，并且设置其各个属性，例如 `author`, `recipient`, `subject`等等。

旋即你需要构建一个`smtp` (或者`smtps`)类。`message`对象必需通过这个`smtp`连接进行发送。

```cpp
message msg;
msg.from(mail_address("mailio library", "mailio@gmail.com"));
msg.add_recipient(mail_address("mailio library", "mailio@gmail.com"));
msg.subject("图样的SMTP邮件");
msg.content("谈笑风生");

smtps conn("smtp.gmail.com", 587);
conn.authenticate("mailio@gmail.com", "mailiopass", smtps::auth_method_t::START_TLS);
conn.submit(msg);
```

为了收取邮件，你需要创建一个空白的`message`对象来承载接收到的数据。邮件数据会通过POP3或者IMAP传递，这取决于邮件服务器。
如果要使用POP3，一个`pop3`(或者`pop3s`)实例需被创建。

```cpp
pop3s conn("pop.mail.yahoo.com", 995);
conn.authenticate("mailio@yahoo.com", "mailiopass", pop3s::auth_method_t::LOGIN);
message msg;
conn.fetch(1, msg);
```

IMAP也是相似的。然而因为IMAP辨识文件夹，所以你必须指定：
```cpp
imaps conn("imap.gmail.com", 993);
conn.authenticate("mailio@gmail.com", "mailiopass", imap::auth_method_t::LOGIN);
message msg;
conn.fetch("inbox", 1, msg);
```

更多进阶姿势在`examples`文件夹里。下文为如何编译：

给Gmail用户的提示：你可能需要[注册](https://support.google.com/accounts/answer/6010255) *mailio* 作为一个可信应用。 关注
[Gmail指示](https://support.google.com/accounts/answer/3466521) 来添加 *mailio* 并为协议生成密码。

# 依赖 #

*mailio*库理应在支持C++17、Boost库和CMake的所有平台上奏效。

对于Linux用户，如下配置是历经测试的：
* Ubuntu 20.04.3 LTS.
* Gcc 8.3.0.
* Boost 1.66 with Regex, Date Time available.
* POSIX Threads, OpenSSL and Crypto libraries available on the system.
* CMake 3.16.3

对于FreeBSD用户，如下配置是历经测试的：
* FreeBSD 13.
* Clang 11.0.1.
* Boost 1.72.0 (port).
* CMake 3.21.3.

对于macOS用户，如下配置是历经测试的：
* Apple LLVM 9.0.0.
* Boost 1.66.
* OpenSSL 1.0.2n available on the system.
* CMake 3.16.3.

对于微软Windows用户，如下配置是历经测试的：
* Windows 10.
* Visual Studio 2019 Community Edition.
* Boost 1.71.0.
* OpenSSL 1.0.2t.
* CMake 3.17.3.

对于Cygwin用户，如下配置是历经测试的：
* Cygwin 3.2.0 on Windows 10.
* Gcc 10.2.
* Boost 1.66.
* CMake 3.20.
* LibSSL 1.0.2t and LibSSL 1.1.1f development packages.

对于MinGW用户，如下配置是历经测试的：
* MinGW 17.1 on Windows 10 which comes with the bundled Gcc and Boost.
* Gcc 9.2.
* Boost 1.71.0.
* OpenSSL 1.0.2t.
* CMake 3.17.3.

# 步骤 #

只需要两步即可构建：首先克隆[分支](https://github.com/karastojko/mailio.git),然后使用CMake或者Vcpkg

## CMake ##

确保OpenSSL, Boost, CMake都在PATH中。否则，需要设置CMake参数`-DOPENSSL_ROOT_DIR` 和 `-DBOOST_ROOT`。
Boost必须在有OpenSSL下构建。如果在PATH中不能发现这些库，通过设置环境变量`library-path` 和 `include`来隐式设置。
你只需要在`bootstrap`后运行`b2`脚本。

动态和静态库都会构建于`build`文件夹。如果你需要把库安装到一个特定的路径(例如`/opt/mailio`),设置CMake参数`-DCMAKE_INSTALL_PREFIX`。

其他可行的参数有`BUILD_SHARED_LIBS`(默认开启，开启会构建动态链接库), `MAILIO_BUILD_DOCUMENTATION`(默认开启，会生成Doxygen文档，如果安装了的话), `MAILIO_BUILD_EXAMPLES`(默认开启，构建示例代码)

### Linux, FreeBSD, macOS, Cygwin ###
从终端直接进入项目路径构建，只需运行：
```
mkdir build
cd ./build
cmake ..
make install
```

### 微软 Windows/Visual Studio ###
从命令提示符进入构建，只需运行：
```
mkdir build
cd .\build
cmake ..
```
会创造一个解决方案文件，在Visual Studio(或者MSBuild)中打开即可构建。

### 微软 Windows/MinGW ###
用`open_distro_window.bat`打开命令提示符，只需运行：
```
mkdir build
cd .\build
cmake.exe .. -G "MinGW Makefiles"
make install
```

## Vcpkg ##
用[Vcpkg](https://github.com/microsoft/vcpkg)安装, 只需要运行：
```
vcpkg install mailio
```

# 特性 #
* 递归的MIME消息格式构建和解析
* 识别最常用的MIME头，例如subject，recipients，content
* 所有编码格式都支持，例如7bit，8bit，二进制，Base64和QP
* Subject, attachment和name部分可以用ascii和utf8编码。
* 所有的媒体格式都可以被识别，包括嵌入的MIME消息。
* MIME消息有可配置的行长度策略和解析的严格模式
* 包含无加密、SSL、TLS等版本的，用于发送消息的SMTP实现
* 包含无加密、SSL、TLS等版本的，用于接受和删除邮件，获得邮箱统计数据的POP3实现
* 包含无加密、SSL、TLS等版本的，用于接受和删除邮件，获得邮箱统计数据，管理文件夹的IMAP实现

# 议题 #
并非所有邮件服务器都被覆盖测试了。如果你发现了什么不能用的库，那么请联系我们。这里是已知的，会在未来修复的问题：

* 无ASCII附件名称为UTF-8

# 贡献者 #
* [Trevor Mellon](https://github.com/TrevorMellon): 提供了CMake构建脚本
* [Kira Backes](mailto:kira.backes[at]nrwsoft.de): 修复了默认邮件日期
* [sledgehammer_999](mailto:hammered999[at]gmail.com): 用Boost随机库替换了std随机库
* [Paul Tsouchlos](mailto:developer.paul.123[at]gmail.com): 更新了构建脚本
* [Anton Zhvakin](mailto:a.zhvakin[at]galament.com): 替换了被弃用的Boost Asio API
* [terminator356](mailto:termtech[at]rogers.com): 用UID搜索获取IMAP消息
* [Ilya Tsybulsky](mailto:ilya.tsybulsky[at]gmail.com): 解决了MIME解析和格式化问题，提供了POP3的UIDL命令
* [Ayaz Salikhov](https://github.com/mathbunnyru): 提供了Conan包管理器
* [Tim Lukas Harken](tlh[at]tlharken.de): 移除了编译警告
* [Rainer Sabelka](mailto:saba[at]sabanet.at]): 提供了接受邮件时的SMTP回执
* [David Garcia](mailto:david.garcia[at]antiteum.com): 提供了Vcpkg支持
* [ImJustStokedToBeHere](https://github.com/ImJustStokedToBeHere): 解决了IMAP中的打字错误
* [lifof](mailto:youssef.beddad[at]gmail.com): 提供了MinGW编译支持
* [Canyon E](https://github.com/canyone2015): 解决了IMAP文件夹定界符的静态变量问题
* [ostermal](https://github.com/ostermal): 修复了MIME头中的水平标签的bug
* [MRsoymilk](mailto:313958485[at]qq.com): 修复了发送附件的bug
* [Don Yihtseu](https://github.com/tsurumi-yizhou): 提供了中文文档
* [Orchistro](https://github.com/orchistro): Improving CMake build script.


# 联系方式 #
如果你发现了bug，请给我的邮箱：contact (at) alepho.com 发邮件。苟利mailio生死以，岂因祸福避趋之。
