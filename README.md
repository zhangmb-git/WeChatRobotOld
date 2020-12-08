# 找钢微信助手

## 安装

1. vs2017
```bash
$ https://docs.microsoft.com/en-us/visualstudio/releasenotes/vs2017-relnotes # 手动下载安装，勾选c++即可
```

2. vcpkg
```bash
$ git clone https://github.com/Microsoft/vcpkg.git
$ cd vcpkg
$ bootstrap-vcpkg.bat # 管理员权限运行（管理员打开CMD或者powershell）

# 使用如下
$ vcpkg search boost
$ vcpkg install boost-thread
```

3. depends
```bash
$ vcpkg install boost-thread # 如果失败，可能是网络问题，多试几次
```

## 运行