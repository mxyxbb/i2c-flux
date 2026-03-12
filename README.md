## I2C-Flux
本软件是I2C接口的上位机控制软件，用于I2C/SMBus/PMBus的通信调试  
适配了两款USB-I2C接口：CP2112接口，以及自己写的RPI2C接口  
软件包内容如下，解压到单独文件夹中运行i2c_flux.exe即可  
download：[release](https://github.com/mxyxbb/i2c-flux/releases/tag/basic)  
<img width="746" height="148" alt="image" src="https://github.com/user-attachments/assets/570ea0c0-9349-4b86-99ac-3b636e0f7de8" />
## 1. 接线说明  
##### 使用CP2112接口：  
  1. 测试板正常上电  
  2. 连接CP2112到电脑  
  3. 连接CP2112与测试板

注：CP2112模块的两排排针并非同一网络，接线时不要接错  
##### 使用RPI2C接口，参考以下链接：
[RPI2C接口](https://github.com/mxyxbb/RP2040-I2C-USB-Adaptor/blob/main/README.md)

## 2. 本软件支持的操作  
支持地址扫描，简单调试，  
还支持创建多个命令组，每个命令组包括以下功能项：  
- 寄存器列表读取  
- 单次操作序列  
- 周期操作序列

支持数据格式转换和数据曲线显示等功能。  
数据格式支持依据公式在raw数据和解析值之间转换  
软件内公式预设的变量包括：  
- b0,b1,b2,...,b31 : raw数据第N个字节(byte)  
- w0,w1,w2...,w15 : raw数据第N个字(word)，小端字，w0 = (b1<<8)|b0

公式支持的运算符包括：  
- 本软件的应用场景下，主要用加减乘除，还支持很多其它公式，详见下面链接文档  
- [ExprTk公式语法]()
- 额外还支持str解析，直接将raw解析为ascii字符（读取），或者将字符串解析到raw（写入）

## 3. 调试软件使用
操作窗口分为简单窗口和多命令表窗口，通过菜单栏的窗口选项控制显示，多个窗口打开时支持拖动布局。  
绘图窗口通过菜单栏的绘图选项开启。  
<<img width="1228" height="739" alt="image" src="https://github.com/user-attachments/assets/3e6a8800-fb2f-4027-b04e-d2f237ac5cec" />

<img width="1234" height="742" alt="image" src="https://github.com/user-attachments/assets/4847408d-c43e-4331-b361-9cecfade50df" />

先介绍简单窗口  
<img width="645" height="517" alt="image" src="https://github.com/user-attachments/assets/45bd23e1-fc90-47c8-a7bd-c959b0c35b3a" />  
### 3.1 简单窗口
#### 3.1.1 连接设备（CP2112接口）
1. 打开连接，软件自动根据vid/pid连接CP2112，设备名称显示的是CP2112芯片的序列号。  
2. 如需修改波特率，需要点击断开按钮，修改波特率，随后重新连接  
3. 目前仅支持100kHz、400kHz两个波特率设置
<img width="444" height="190" alt="image" src="https://github.com/user-attachments/assets/c77239fd-0dc4-42bd-bf8b-aa44e94f051e" />
<img width="450" height="188" alt="image" src="https://github.com/user-attachments/assets/85ebc42e-b4c4-423b-b797-684cb47d7e1a" />  

#### 3.1.2 扫描从机
1. 扫描从机功能，可以扫描I2C总线上的从机设备，扫描完成后列在下面表格中
2. 扫描方式：通过发送一个I2C地址读，查看是否有从设备响应，来确认该地址是否有PMBus从机
3. 点一下选择框，可以快速地将简单窗口通信使用的从机地址设置为对应地址
<img width="450" height="156" alt="image" src="https://github.com/user-attachments/assets/96b481e3-0425-4c56-9b35-d5e94d8b7a99" />
<img width="439" height="156" alt="image" src="https://github.com/user-attachments/assets/ee66ccc9-0c98-4942-aba9-e09a079d1fa0" />
<img width="438" height="151" alt="image" src="https://github.com/user-attachments/assets/c5599f4a-ca56-4a32-ba6d-f444259fdc0c" />

#### 3.1.3 简单操作
1. 读取操作，填入从机地址（可以直接输入），寄存器地址，和读取长度，即可执行操作。
2. 写入操作，填入从机地址（可以直接输入），寄存器地址，和写入的数据，即可执行操作。自动计算写入长度。
3. 发命令，填入从机地址（可以直接输入），寄存器地址，填写好后，执行操作即可。寄存器地址即命令码。

<img width="407" height="183" alt="image" src="https://github.com/user-attachments/assets/523e0382-73c3-4129-8610-441b692edfd0" />
<img width="421" height="171" alt="image" src="https://github.com/user-attachments/assets/915c1709-18a8-45d5-8017-b7ee189a6f26" />
<img width="400" height="156" alt="image" src="https://github.com/user-attachments/assets/26270bfc-0efa-4897-8ae5-f3a57c39bd7f" />

### 3.2 多命令表窗口
一个命令(表)组包括三个子命令表：寄存器表、单次触发表、周期触发表  
关闭软件或按下Ctrl+S快捷键时，软件会自动保存命令表数据，启动时自动载入   
<img width="645" height="352" alt="image" src="https://github.com/user-attachments/assets/fb1ff773-e0e1-42d5-8db8-5725bcd0123a" />

#### 3.2.1 寄存器表操作
1. 首先请手动填写/确认从机地址
2. 寄存器表的功能是按顺序读取列表中的所有寄存器值，设计目的是查看一系列寄存器的原始值
3. 寄存器描述是可选项
4. 复制、移动和删除前，请先在序号列选中要操作的行
5. 添加时，无选中则添加在末尾，有选中则添加在选中行之后
6. 属性选项：包含覆写从机地址功能，允许为单条命令指定其它的从机地址
7. 寄存器表中，右键点击添加按钮，支持快捷添加功能，分隔符请使用英文逗号
8. 点击读取所有寄存器即可执行一次列表读取
<img width="645" height="300" alt="image" src="https://github.com/user-attachments/assets/6f67e515-8a44-4e39-8bbb-99295232c53c" />
<img width="600" height="322" alt="image" src="https://github.com/user-attachments/assets/2ec4b05a-8690-45ff-9cfc-4d89788a482e" />
<img width="669" height="395" alt="image" src="https://github.com/user-attachments/assets/d9ddd88a-2c00-48c4-b572-ea2f7ca877a1" />

#### 3.2.2 单次触发表操作
1. 首先请手动填写/确认从机地址
2. 单次触发表的功能是单次按顺序执行列表中的读写操作，设计目的单次读写测试
3. 复制、移动和删除前，请先在序号列选中要操作的行
4. 添加时，无选中则添加在末尾，有选中则添加在选中行之后
5. 条目使能：首列的勾选框可以设定该条命令是否执行
6. 延时配置：可配置操作后的延时，单位为ms，由于windows和CP2112接口的限制，100ms以内精度有限
7. 命令类型：写入、读取、命令（即仅发送1字节命令码）；命令类型下仅reg地址参数有效，不使用其它参数
8. 解析选项：可配置读取后，用来解析数据的读取公式；或者写入前，通过写入公式将解析值转换为raw值，用来写入。具体公式内容参见解析公式的说明。
9. 单条执行：可以仅执行单条命令
10. 属性选项：包含覆写从机地址功能，允许为单条命令指定其它的从机地址
11. 单次触发表中，右键点击添加按钮，支持快捷添加功能，分隔符请使用英文逗号
12. 点击执行所有命令即可执行一次命令列表
<img width="656" height="343" alt="image" src="https://github.com/user-attachments/assets/a3b70fd2-f163-4cb1-b036-0c192ffcecae" />
<img width="831" height="400" alt="image" src="https://github.com/user-attachments/assets/01eb05cd-e7d2-418e-a4ae-8709fe31dd4d" />

#### 3.2.3 周期触发表操作
1. 首先请手动填写/确认从机地址和周期间隔，执行时间短于间隔时间时，剩余时间空转等待，执行时间较长时，仍会执行全部命令，执行周期消耗的时间会大于设定的周期间隔。
2. 周期触发表的功能是周期性顺序执行列表中的读写操作，设计目的是连续读写测试
3. 复制、移动和删除前，请先在序号列选中要操作的行
4. 添加时，无选中则添加在末尾，有选中则添加在选中行之后
5. 条目使能：首列的勾选框可以设定该条命令是否执行
6. 延时配置：可配置操作后的延时，单位为ms，由于windows和CP2112接口的限制，100ms以内精度有限
7. 命令类型：写入、读取、命令（即仅发送1字节命令码）；命令码类型下仅reg地址参数有效，不使用其它参数
8. 解析选项：可配置读取后，用来解析数据的读取公式；或者写入前，通过写入公式将解析值转换为raw值，用来写入。具体公式内容参见解析公式的说明。
9. 绘图勾选框：解析值后勾选框，勾选时，会在开始周期触发按钮按下时，创建绘图通道，将读取数据的解析值绘制在绘图窗口中。通道名称优先使用别名，如果没有别名则使用 "CH1", "CH2"...。
10. 单条执行：可以仅执行单条命令
11. 属性选项：包含覆写从机地址功能，允许为单条命令指定其它的从机地址
12. 单次触发表中，右键点击添加按钮，支持快捷添加功能，分隔符请使用英文逗号
13. 点击开始周期触发即可开始执行
<img width="656" height="343" alt="image" src="https://github.com/user-attachments/assets/ca86eafe-7006-4719-85c9-600f031ac362" />
<img width="872" height="393" alt="image" src="https://github.com/user-attachments/assets/590cf7b8-6b51-4cd8-b546-630cb6486a19" />

https://github.com/user-attachments/assets/5aca0e86-a72c-455c-96b5-92937a1bf188



#### 3.2.4 周期触发的数据记录
开始记录按钮会将接收到的数据记录到指定路径下的csv文件中，重复记录时如果和已有文件名重复，会不询问直接覆盖掉当前文件。  
<img width="453" height="395" alt="image" src="https://github.com/user-attachments/assets/61cb5d4a-e6cd-428d-931a-e069d086110d" />
<img width="455" height="309" alt="image" src="https://github.com/user-attachments/assets/436b4cca-c4f9-460e-8233-436c33fb7a0d" />

### 3.3 绘图窗口
通过菜单->绘图->两个选项，打开绘图窗口和绘图设置窗口，  
绘图窗口可以自动缩放和暂停滚动  
目前绘图设置仅支持  
- 通道显示
- 颜色修改
- 纵向缩放

## 5. 补充-解析负数
由于exprtk的脚本支持，解析式可以输入以下表达式，以解析负数(如 0xff 0xff)  
        if(w0>800*32) (w0-65536)/32;else w0/32;

## 6. 扫描不到地址时的排查杂项
- 重新插拔CP2112接口模块
- I2C口的上拉电源
- 重新使能，复位一下mcu

## TODO List
- 界面优化：将解析公式按钮移动到解析值的右键选项中，
- 交互优化：将绘图勾选框逻辑复用使能勾选框的逻辑
- 功能新增：增加CRC功能，添加在多命令表的地址输入框右侧，checkbox文本“使能CRC校验”,然后后面带一个设置按钮，可以设定crc选项
- 功能新增：可将当前绘图缓冲区数据导出为CSV格式
- 交互优化：在周期触发表格中，鼠标悬浮在寄存器地址区域上方时，出现tooltip显示出该寄存器地址的描述，来源自寄存器表格的描述字符串。
- 功能新增：增加日志功能
- 接口兼容：增加其它I2C接口的支持
- 功能新增：增加IO操作
