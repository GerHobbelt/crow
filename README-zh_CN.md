![��ѻ��־](http://i.imgur.com/wqivvjK.jpg)

��ѻ��Web��C++΢�����ܣ�֧��mac,linux,windows,����ƽ̨�������ٶ������Ѹ�������ң���һ��������֧�����ݿ⣬�Լ�ORM��

��������� Python Flask��[��Asciphx�ṩ�ķ�֧]

[![Travis Build](https://travis-ci.org/ipkn/crow.svg?branch=master)](https://travis-ci.org/ipkn/crow)
[![Coverage Status](https://coveralls.io/repos/ipkn/crow/badge.svg?branch=master)](https://coveralls.io/r/ipkn/crow?branch=master)

```c++
#include "crow.h"
int main(){
    crow::SimpleApp app;
    CROW_ROUTE(app, "/")([](){
        return "Hello world";
    });
    app.port(18080).multithreaded().run();
}
```

## �ص�
- ����·�ɣ�������Python Flask
- ���Ͱ�ȫ������򣨲μ�ʾ�������ǳ���
 ![��׼���](./Benchmark.png)
- �������[crow benchmark]������(https://github.com/ipkn/crow-benchmark)
- ��������JSON��������crow:��JSON��
- Ҳ����ʹ��[json11](https://github.com/dropbox/json11)��[rapidjson](https://github.com/miloyip/rapidjson)Ϊ�˸��õ��ٶȻ�ɶ���
- [Mustache](http://mustache.github.io/)����ģ��⣨crow:��mustache��
- ��ҳü��ÿһ��� [`crow_all.h`](https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h) with every features ([Download from here](https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h))
- �м��֧�֣�Websocket֧��
- ֧�־�̬��Դ,����Ĭ����'static/'Ŀ¼
## ���ڿ�����
-~~����ORM~~
-���[sqlpp11](https://github.com/rbock/sqlpp11)�������Ҫ�Ļ���

## ʾ��

#### �������Ⱦ
```c++
  CROW_ROUTE(app,"/")([] {
	char name[256];gethostname(name,256);
	mustache::Ctx x;x["servername"]=name;
	auto page=mustache::load("index.html");
	return page.render(x);
  });
```

#### JSON��Ӧ
```c++
CROW_ROUTE(app, "/json")([]{
    crow::json::wvalue x;
    x["message"] = "Hello, World!";
    return x;
});
```

#### �۾�
```c++
CROW_ROUTE(app,"/hello/<int>")([](int count){
    if (count > 100) return crow::Res(400);
    std::ostringstream os;
    os << count << " bottles of beer!";
    return crow::Res(os.str());
});
```
����ʱ�Ĵ������������ͼ�� 
```c++
// ���������Ϣ"�������������URL������ƥ��"
CROW_ROUTE(app,"/another/<int>")([](int a, int b){
    return crow::Res(500);
});
```

#### ����JSON����
```c++
CROW_ROUTE(app, "/add_json").methods("POST"_method)
([](const crow::Req& req){
    auto x = crow::json::load(req.body);
    if (!x)
        return crow::Res(400);
    int sum = x["a"].i()+x["b"].i();
    std::ostringstream os;
    os << sum;
    return crow::Res{os.str()};
});
```

## ��ι���
�����ֻ��ʹ��crow���븴��amalgamate/crow_all.h ����������

### Ҫ��
- C++ ��������֧��C++ 11����G++����>=4.8��
- �κΰ汾��boost��
- ����ʾ����CMake
- ������tcmalloc/jemalloc����������ٶȡ�
- ����֧��VS2019���������ޣ�ֻ��url������ʱ�����á���

### ���������ԡ�ʾ����
����ʹ��CMake����Դ�����⹹����
```
mkdir build
cd build
cmake ..
make
```

����ʹ�������������в��ԣ�

```
ctest
```

### ��װȱ�ٵ�������
#### Ubuntu
    sudo apt-get install build-essential libtcmalloc-minimal4 && sudo ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so
#### OSX
    brew install boost google-perftools

#### windows

>�״ΰ�װboost

>�ڶ����޸�CmakeLists.txt

##### CmakeLists.txt[ʾ��]

```cmake
SET(BOOST_ROOT "E:/Code/boost_1_75_0") #Installation address of decompressed version
set(Boost_USE_STATIC_LIBS ON) #Support anything else
```

### ����
Crowʹ�����¿⡣  
http������ https://github.com/nodejs/http-parser

http_parser.c ����NGINX��Ȩ���������������Ү��� src/http/ngx_http_parse.c 
�������ĵ����������NGINX�Ͱ�Ȩ����Joyent��Inc.�������ڵ㹱���ߡ���Ȩ���С�
�����׼���κ���ȡ�ø���
�����������ĵ��ļ����������������
�������Ƶؾ�Ӫ�����������������
ʹ�á����ơ��޸ġ��ϲ����������ַ�������ɺ�/��
��������ĸ����������������ʹ����
�������������ṩ��
������Ȩ�����ͱ��������Ӧ������
��������и�������Ҫ���֡�
���������ԭ�����ṩ�����ṩ�κ���ʽ����ʾ����ʾ����
Ĭʾ�������������������Ա�֤��
�������ض�Ŀ�ĺͷ��ַ��ԡ����κ������
���߻��Ȩ�����˶��κ����⡢�𺦻�����
���Σ��������ں�ͬ���ϡ���Ȩ���ϻ����������в�����
���ԡ����Ի��������ʹ�û����������й�
������С�

qs_parse https://github.com/bartgrantham/qs_parse  
��Ȩ���У�c��2010 Bart Grantham
�����׼���κ���ȡ�ø���
�����������ĵ��ļ���������������Դ���
������в������ƣ�������������Ȩ��
ʹ�á����ơ��޸ġ��ϲ����������ַ�������ɺ�/������
����ĸ�����������ʹ���������Ա
�������������ṩ��
������Ȩ�����ͱ��������Ӧ������
��������и�������Ҫ���֡�

TinySHA1 https://github.com/mohaps/TinySHA1

TinySHA1-SHA1�㷨��һ��ֻ������ͷ��ʵ�֡�����boost::uuid::details�е�ʵ��
Cmohaps@gmail.com
�ش���������κ�Ŀ��ʹ�á����ơ��޸ĺͷַ����������ɣ������Ƿ��շѣ�ǰ����������Ȩ�����ͱ�����������������и����С�
���������ԭ�����ṩ�����߲��е��뱾����йص����б�֤�������������Ժ������Ե����а�ʾ��֤��һ