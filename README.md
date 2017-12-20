# doorphone
一个简单的智能门禁系统
 
### 功能
* 两台GEC210互联，一台充当洋楼大堂入口的**客户端**，一台充当业主家里的**业主端**
* **客户端**可进行的操作：
    * 输入房间号，激活监控机的求访铃声。（视频自动开启，且无法关闭）
    * 关闭对讲系统。（视频自动关闭）
* **业主端**可进行的操作：
    * 点击“监控”，可单向看到并听到大堂的实时图像和声音。
    * 点击“语音对讲”，可实时对讲，再次点击则关闭此功能。
    * 点击“视频对讲”，可实时视频对讲，再次点击则关闭此功能。
    * 点击“开门”，向来客机发送一串特定控制符，对讲结束。

### 编程环境
* Ubuntu16.04
* [GEC210开发板](https://item.taobao.com/item.htm?spm=a1z10.1-c.w5003-6427619857.1.45ee61bboSErWr&id=38029903389&scene=taobao_shop)
* JPEG制式摄像头

### 技术点
* 嵌入式C语言
* Linux文件IO
* 输入子系统、触摸屏操作
* LCD工作原理
* JPG/JPEG图像解码
* V4L2视频子系统
* Linux音频核心：ALSA库编译与部署
* 电子抓拍
* TCP/IP协议，经典socket编程

### 技术支持
* <a href="https://weidian.com/?userid=260920190">![image](https://github.com/vincent040/lab/blob/master/resources/weidian.jpg?raw=true)
* <a href="//shang.qq.com/wpa/qunwpa?idkey=bc2c3338276a40ac72131230ad041a00c60a2fe45172ab6b9a93fea44cf0e6fa">![image](https://github.com/vincent040/lab/blob/master/resources/QQ_qun.png?raw=true) 
