# 🖱️ Driver USB para Mouse com Joystick - Raspberry Pi Pico W

**Disciplina:** Tópicos em Sistemas Embarcados 2 (TPSE2)  
**Projeto:** Driver Linux para Periférico USB   
**Hardware:** Raspberry Pi Pico W + Joystick Analógico + LED RGB

---

## 📋 Sumário

1. [Visão Geral](#visão-geral)
2. [Requisitos Atendidos](#requisitos-atendidos)
3. [Arquitetura do Sistema](#arquitetura-do-sistema)
4. [Hardware](#hardware)
5. [Compilação e Instalação](#compilação-e-instalação)
6. [Demonstração](#demonstração)
7. [Protocolo de Comunicação](#protocolo-de-comunicação)
8. [Estrutura do Código](#estrutura-do-código)

---

## 🎯 Visão Geral

Este projeto implementa um **driver Linux completo** para um dispositivo USB customizado baseado em Raspberry Pi Pico W. O dispositivo funciona como:

- **Mouse HID padrão** (Interface 0) - Reconhecido automaticamente pelo sistema operacional
- **Dispositivo Vendor customizado** (Interface 1) - Controlado pelo driver Linux desenvolvido

### Funcionalidades Principais

✅ **Driver Kernel Linux** com operações de **READ** e **WRITE**  
✅ **Firmware TinyUSB** completo com USB Composite Device  
✅ **Aplicação Userspace** para controle de LEDs e monitoramento de eventos  
✅ **Mouse funcional** com joystick analógico de 2 eixos  
✅ **3 botões físicos** (esquerdo, direito, meio)  
✅ **LED RGB controlável** via comandos USB  
✅ **Feedback visual** em tempo real

---

## ✅ Requisitos Atendidos

### 1. Driver Linux com READ/WRITE ✓

**Arquivo:** `driver/pico_mouse_driver.c`

- ✅ **Operação WRITE**: Host → Device (controle de LED)
- ✅ **Operação READ**: Device → Host (eventos de botões)
- ✅ **Bulk Transfer** USB com timeout de 2 segundos
- ✅ **Mutex** para proteção contra condições de corrida
- ✅ **Estatísticas** de pacotes enviados/recebidos/erros

```c
static ssize_t pico_mouse_write(struct file *file, const char __user *user_buf,
                                size_t count, loff_t *ppos);

static ssize_t pico_mouse_read(struct file *file, char __user *user_buf,
                               size_t count, loff_t *ppos);
```

### 2. Firmware do Dispositivo ✓

**Arquivos:** `firmware/main.c`, `firmware/usb_descriptors.c`

- ✅ **USB Composite Device** (HID + Vendor)
- ✅ **TinyUSB Stack** completo
- ✅ **Descritores USB** customizados
- ✅ **Calibração automática** do joystick
- ✅ **Callbacks USB** implementados

### 3. Aplicação de Usuário ✓

**Arquivo:** `userapp/pico_mouse_app.c`

- ✅ Controle de LED RGB via linha de comando
- ✅ Monitor de eventos de botões em tempo real
- ✅ Sequência de testes automatizada
- ✅ Interface intuitiva com feedback visual

---

## 🏗️ Arquitetura do Sistema

```
┌────────────────────────────────────────────────────────────┐
│                      LINUX HOST                             │
│                                                             │
│  ┌─────────────────┐         ┌─────────────────────┐      │
│  │   HID Driver    │         │   Custom Driver     │      │
│  │  (kernel padrão)│         │ pico_mouse_driver.ko│      │
│  └────────┬────────┘         └──────────┬──────────┘      │
│           │                             │                  │
│           │ Interface 0                 │ Interface 1      │
│           │ (HID Mouse)                 │ (Vendor)         │
│           │                             │                  │
│           │ ┌─────────────────────────┐ │                  │
│           │ │  /dev/pico_mouse0       │ │                  │
│           │ │  read() / write()       │ │                  │
│           │ └─────────────────────────┘ │                  │
│           │                             │                  │
│  ┌────────▼─────────────────────────────▼────────┐        │
│  │        USB Subsystem (libusb)                 │        │
│  └───────────────────────┬───────────────────────┘        │
└──────────────────────────┼────────────────────────────────┘
                           │
                           │ USB Cable
                           ▼
┌────────────────────────────────────────────────────────────┐
│              RASPBERRY PI PICO W (Firmware)                 │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  TinyUSB Stack (USB Composite Device)               │  │
│  │  • VID: 0xCAFE, PID: 0x4003                        │  │
│  │  • Interface 0: HID Mouse (EP 0x81)                │  │
│  │  • Interface 1: Vendor (EP 0x02 OUT, 0x82 IN)     │  │
│  └───────────────────┬──────────────────────────────────┘  │
│                      │                                      │
│  ┌───────────────────▼──────────────────────────────────┐  │
│  │  Application Logic (main.c)                         │  │
│  │  • mouse_task() - Processa joystick e botões       │  │
│  │  • vendor_task() - Envia eventos USB               │  │
│  │  • tud_vendor_rx_cb() - Recebe comandos LED        │  │
│  └───────────────────┬──────────────────────────────────┘  │
│                      │                                      │
│  ┌───────────────────▼──────────────────────────────────┐  │
│  │  Hardware (GPIO/ADC/PWM)                            │  │
│  │  • Joystick: GPIO 26/27 (ADC)                      │  │
│  │  • Botões: GPIO 5/6/10                             │  │
│  │  • LED RGB: GPIO 11/12/13 (PWM)                    │  │
│  └─────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

---

## 🔧 Hardware

### Componentes Necessários

| Item | Quantidade | Especificação |
|------|------------|---------------|
| Raspberry Pi Pico W | 1 | Microcontrolador RP2040 com WiFi |
| Joystick Analógico | 1 | 2 eixos + botão (tipo PS2) |
| Botões Tácteis | 2 | Push buttons 6x6mm |
| LED RGB | 1 | Ânodo comum ou cátodo comum |
| Resistores 220Ω | 3 | Para LEDs |
| Protoboard | 1 | 400 ou 830 pontos |
| Jumpers | ~15 | Macho-macho |
| Cabo USB Micro | 1 | Para programação e operação |

### Pinagem

```
┌─────────────────────────────────────────┐
│         Raspberry Pi Pico W              │
├─────────────────────────────────────────┤
│  GPIO 26 (ADC0)  → Joystick VRx (X)    │
│  GPIO 27 (ADC1)  → Joystick VRy (Y)    │
│  GPIO 10         → Botão Esquerdo (A)   │
│  GPIO 5          → Botão Direito (B)    │
│  GPIO 6          → Botão Meio (Joy SW)  │
│  GPIO 13 (PWM6B) → LED RGB - Vermelho   │
│  GPIO 11 (PWM5B) → LED RGB - Verde      │
│  GPIO 12 (PWM6A) → LED RGB - Azul       │
│  3V3 (OUT)       → VCC Joystick         │
│  GND             → GND Comum            │
└─────────────────────────────────────────┘
```

### Diagrama de Conexão

```
        Joystick              Pico W           LED RGB
        ┌──────┐              ┌──────┐         ┌─────┐
    VCC─┤+  SW ├──────────────┤GP6   │         │  R  ├─220Ω─GP13
        │  VRx├───────────────┤GP26  │         │  G  ├─220Ω─GP11
        │  VRy├───────────────┤GP27  │         │  B  ├─220Ω─GP12
    GND─┤─    │               │      │      ┌──┤ COM │
        └──────┘               │3V3   ├──────┘  └─────┘
                               │GND   ├─────────────┐
   Botão A                     │GP10  │             │
     ├──────────────────────────┤      │            GND
    GND                         │GP5   │
                                └──────┘
   Botão B                         │
     ├────────────────────────────┘
    GND
```

---

## 🚀 Compilação e Instalação

### Pré-requisitos

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    build-essential \
    linux-headers-$(uname -r)

# Pico SDK
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=~/pico-sdk
```

### 1. Compilar Firmware

```bash
cd firmware/
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Resultado: pico_mouse_joystick.uf2
```

### 2. Gravar Firmware no Pico

```bash
# Método BOOTSEL:
# 1. Segure o botão BOOTSEL no Pico W
# 2. Conecte o USB (ainda segurando)
# 3. Solte o botão - aparece drive RPI-RP2
# 4. Copie o arquivo:

cp pico_mouse_joystick.uf2 /media/$USER/RPI-RP2/

# O Pico reinicia automaticamente
```

### 3. Compilar Driver Linux

```bash
cd driver/

# Criar Makefile
cat > Makefile << 'EOF'
obj-m += pico_mouse_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod pico_mouse_driver.ko

remove:
	sudo rmmod pico_mouse_driver
EOF

# Compilar
make

# Resultado: pico_mouse_driver.ko
```

### 4. Compilar Aplicação Userspace

```bash
cd userapp/

# Criar Makefile
cat > Makefile << 'EOF'
CC = gcc
CFLAGS = -Wall -O2
TARGET = pico_mouse_app

all: $(TARGET)

$(TARGET): pico_mouse_app.c
	$(CC) $(CFLAGS) -o $(TARGET) pico_mouse_app.c

clean:
	rm -f $(TARGET)
EOF

# Compilar
make

# Resultado: pico_mouse_app
```

---

## 🎬 Demonstração

### Passo 1: Verificar Dispositivo USB

```bash
# Conectar Pico programado no USB
# Verificar reconhecimento
lsusb | grep -i cafe

# Saída esperada:
# Bus 001 Device 015: ID cafe:4003 TPSE2 Lab Pico Mouse Joystick Composite

# Ver detalhes
lsusb -v -d cafe:4003 | grep -E "idVendor|idProduct|bInterface"
```

### Passo 2: Carregar Driver

```bash
cd driver/
sudo insmod pico_mouse_driver.ko

# Verificar carregamento
lsmod | grep pico_mouse

# Ver mensagens do kernel
dmesg | tail -15

# Saída esperada:
# pico_mouse: EP IN = 0x82
# pico_mouse: EP OUT = 0x02
# pico_mouse: Pico Mouse connected on /dev/pico_mouse0
```

### Passo 3: Configurar Permissões

```bash
# Verificar device node
ls -l /dev/pico_mouse0

# Dar permissão (temporário)
sudo chmod 666 /dev/pico_mouse0

# OU criar regra udev permanente
echo 'KERNEL=="pico_mouse[0-9]*", MODE="0666"' | \
sudo tee /etc/udev/rules.d/99-pico-mouse.rules

sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Passo 4: Testar Funcionalidades

#### 4.1 Mouse HID (Interface 0)

```bash
# O mouse já funciona automaticamente!
# Mova o joystick → cursor se move
# Clique os botões → funciona como mouse normal

# Verificar input devices
cat /proc/bus/input/devices | grep -A 10 "Pico Mouse"
```

#### 4.2 Controle de LED (Interface 1 - Driver Customizado)

```bash
cd userapp/

# Testar cores básicas
./pico_mouse_app red
./pico_mouse_app green
./pico_mouse_app blue
./pico_mouse_app off

# Cor customizada (RGB)
./pico_mouse_app custom 128 0 255  # Roxo

# Sequência de teste automática
./pico_mouse_app test
```

#### 4.3 Monitorar Eventos de Botões

```bash
# Terminal 1: Monitor de eventos
./pico_mouse_app monitor

# Pressione os botões no Pico
# Saída esperada:
# [EVENT] 0x10 - LEFT BUTTON PRESSED
# [EVENT] 0x11 - LEFT BUTTON RELEASED
# [EVENT] 0x20 - RIGHT BUTTON PRESSED
# [EVENT] 0x21 - RIGHT BUTTON RELEASED
# [EVENT] 0x30 - MIDDLE BUTTON PRESSED
# [EVENT] 0x31 - MIDDLE BUTTON RELEASED

# Ctrl+C para parar
```

#### 4.4 Logs do Kernel

```bash
# Terminal 2: Monitor de kernel
sudo dmesg -w | grep pico_mouse

# Ao usar aplicação, verá:
# pico_mouse: wrote 1 bytes (cmd=0x01)
# pico_mouse: read 1 bytes (event=0x10)
```

### Passo 5: Desconectar

```bash
# Desconectar Pico fisicamente
dmesg | tail -5

# Saída esperada:
# pico_mouse: disconnecting (stats: sent=15, recv=8, errors=0)

# Remover driver
sudo rmmod pico_mouse_driver

# Verificar remoção
lsmod | grep pico
```

---

## 📡 Protocolo de Comunicação

### USB Composite Device

**VID:** `0xCAFE` (Vendor ID customizado)  
**PID:** `0x4003` (Product ID)

#### Interface 0: HID Mouse (Padrão)
- **Classe:** HID (0x03)
- **Protocolo:** Mouse (0x02)
- **Endpoint IN:** 0x81 (64 bytes)
- **Descritor:** Mouse com 3 botões + XY + Wheel

#### Interface 1: Vendor (Customizada)
- **Classe:** Vendor (0xFF)
- **Endpoint OUT:** 0x02 (64 bytes) - Host → Device
- **Endpoint IN:** 0x82 (64 bytes) - Device → Host

### Comandos de LED (Host → Device via WRITE)

| Comando | Valor | Payload | Descrição |
|---------|-------|---------|-----------|
| `CMD_LED_OFF` | 0x00 | 1 byte | Desligar LED |
| `CMD_LED_RED` | 0x01 | 1 byte | LED vermelho |
| `CMD_LED_GREEN` | 0x02 | 1 byte | LED verde |
| `CMD_LED_BLUE` | 0x03 | 1 byte | LED azul |
| `CMD_LED_YELLOW` | 0x04 | 1 byte | LED amarelo |
| `CMD_LED_CYAN` | 0x05 | 1 byte | LED ciano |
| `CMD_LED_MAGENTA` | 0x06 | 1 byte | LED magenta |
| `CMD_LED_WHITE` | 0x07 | 1 byte | LED branco |
| `CMD_LED_CUSTOM` | 0x08 | 4 bytes | Cor RGB customizada |

**Exemplo de Cor Customizada:**
```
Byte 0: 0x08 (comando)
Byte 1: R (0-255)
Byte 2: G (0-255)
Byte 3: B (0-255)
```

### Eventos de Botões (Device → Host via READ)

| Evento | Valor | Descrição |
|--------|-------|-----------|
| `EVENT_BTN_LEFT_PRESS` | 0x10 | Botão esquerdo pressionado |
| `EVENT_BTN_LEFT_RELEASE` | 0x11 | Botão esquerdo solto |
| `EVENT_BTN_RIGHT_PRESS` | 0x20 | Botão direito pressionado |
| `EVENT_BTN_RIGHT_RELEASE` | 0x21 | Botão direito solto |
| `EVENT_BTN_MID_PRESS` | 0x30 | Botão meio pressionado |
| `EVENT_BTN_MID_RELEASE` | 0x31 | Botão meio solto |

---

## 📂 Estrutura do Código

### Firmware (Raspberry Pi Pico)

```
firmware/
├── main.c                 # Lógica principal (444 linhas)
│   ├── mouse_task()      # Processa joystick e botões (30Hz)
│   ├── vendor_task()     # Envia eventos USB
│   ├── tud_vendor_rx_cb()# Recebe comandos LED
│   └── main()            # Inicialização e loop
│
├── usb_descriptors.c      # Descritores USB (125 linhas)
│   ├── desc_device       # Device descriptor
│   ├── desc_configuration# Config descriptor (HID + Vendor)
│   ├── hid_report_descriptor # Mouse HID report
│   └── callbacks TinyUSB # Descritores dinâmicos
│
├── tusb_config.h          # Configuração TinyUSB
│   ├── CFG_TUD_HID = 1   # Habilita HID
│   └── CFG_TUD_VENDOR = 1# Habilita Vendor
│
└── CMakeLists.txt         # Build configuration
```

### Driver Linux (Kernel Module)

```
driver/
├── pico_mouse_driver.c    # Driver kernel (300+ linhas)
│   ├── pico_mouse_write()# Envia comandos LED
│   ├── pico_mouse_read() # Lê eventos de botões
│   ├── pico_mouse_probe()# Inicializa dispositivo
│   └── pico_mouse_disconnect() # Cleanup
│
└── Makefile               # Build kernel module
```

### Aplicação Userspace

```
userapp/
├── pico_mouse_app.c       # Aplicação CLI (400+ linhas)
│   ├── send_led_command()# Envia comandos via write()
│   ├── monitor_events()  # Lê eventos via read()
│   └── run_test_sequence() # Testes automatizados
│
└── Makefile               # Build userspace app
```

---

## 🎓 Conceitos Demonstrados

### 1. USB Device Programming
- ✅ USB Composite Device (múltiplas interfaces)
- ✅ HID Class (Human Interface Device)
- ✅ Vendor Class (comunicação customizada)
- ✅ Bulk Transfer (read/write de dados)
- ✅ USB Descriptors (device, configuration, interface, endpoint)

### 2. Linux Kernel Module
- ✅ Character device driver
- ✅ File operations (open, release, read, write)
- ✅ USB subsystem integration
- ✅ Mutex para sincronização
- ✅ Error handling e logging

### 3. Embedded Firmware
- ✅ TinyUSB stack
- ✅ ADC para leitura analógica (joystick)
- ✅ GPIO para botões digitais
- ✅ PWM para controle de LED RGB
- ✅ Event-driven architecture

### 4. User-Kernel Communication
- ✅ Device files (/dev/pico_mouse0)
- ✅ System calls (read/write)
- ✅ Error propagation (errno)
- ✅ Copy to/from user space

---

## 📊 Fluxo de Dados Completo

```
┌─────────────────────────────────────────────────────────┐
│ 1. WRITE: Controle de LED                               │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  pico_mouse_app                                          │
│       │ write(fd, {0x01}, 1)  // CMD_LED_RED            │
│       ▼                                                  │
│  /dev/pico_mouse0                                        │
│       │                                                  │
│       ▼                                                  │
│  pico_mouse_driver.ko                                    │
│       │ copy_from_user()                                 │
│       │ usb_bulk_msg(ep_out=0x02)                        │
│       ▼                                                  │
│  USB Subsystem                                           │
│       │                                                  │
│       ▼ Bulk OUT Transfer                               │
│  Pico W - Interface 1                                    │
│       │ tud_vendor_rx_cb()                               │
│       │ handle_led_command(0x01)                         │
│       ▼                                                  │
│  set_rgb_color(255, 0, 0)                                │
│       │                                                  │
│       ▼ PWM                                              │
│  LED RGB acende VERMELHO ✅                              │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ 2. READ: Eventos de Botões                              │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  Usuário pressiona botão físico                          │
│       │ GPIO 10 = LOW                                    │
│       ▼                                                  │
│  Pico W - mouse_task()                                   │
│       │ !gpio_get(BUTTON_LEFT_PIN)                       │
│       │ event_push(EVENT_BTN_LEFT_PRESS)                 │
│       ▼                                                  │
│  vendor_task()                                           │
│       │ event_pop()                                      │
│       │ tud_vendor_write({0x10}, 1)                      │
│       ▼ Bulk IN Transfer                                │
│  USB Subsystem                                           │
│       │                                                  │
│       ▼                                                  │
│  pico_mouse_driver.ko                                    │
│       │ usb_bulk_msg(ep_in=0x82)                         │
│       │ copy_to_user()                                   │
│       ▼                                                  │
│  /dev/pico_mouse0                                        │
│       │                                                  │
│       ▼                                                  │
│  pico_mouse_app                                          │
│       │ read(fd, buf, 64)                                │
│       │ buf[0] = 0x10                                    │
│       ▼                                                  │
│  printf("LEFT BUTTON PRESSED") ✅                        │
└─────────────────────────────────────────────────────────┘
```

---

## 🔧 Configuração Avançada

### Ajustar Sensibilidade do Mouse

Edite `firmware/main.c`:

```c
#define DEADZONE          150   // Zona morta (50-300)
#define SENSITIVITY        20   // Divisor (10-50)
#define MAX_SPEED         127   // Velocidade máxima
#define POLLING_RATE       30   // Taxa de atualização (Hz)
```

**Exemplos:**
- Mouse mais rápido: `SENSITIVITY 10`
- Mouse mais preciso: `SENSITIVITY 30`
- Deadzone maior: `DEADZONE 250`

### Inverter Eixos do Joystick

Se o movimento estiver invertido:

```c
// Em mouse_task(), após calcular x_move e y_move:
x_move = -x_move;  // Inverter X
y_move = -y_move;  // Inverter Y
```

---

## 🐛 Troubleshooting

### Problema: Driver não carrega

**Erro:** `insmod: ERROR: could not insert module`

**Solução:**
```bash
# Verificar headers do kernel
ls /lib/modules/$(uname -r)/build

# Se não existir, instalar:
sudo apt-get install linux-headers-$(uname -r)

# Recompilar
cd driver && make clean && make
```

### Problema: Permission denied no /dev/pico_mouse0

**Solução:**
```bash
# Temporária:
sudo chmod 666 /dev/pico_mouse0

# Permanente (udev rule):
echo 'KERNEL=="pico_mouse[0-9]*", MODE="0666"' | \
sudo tee /etc/udev/rules.d/99-pico-mouse.rules
sudo udevadm control --reload-rules
```

### Problema: Mouse não move

**Verificar:**
1. Conexões do joystick (VRx→GPIO26, VRy→GPIO27)
2. Alimentação 3.3V no joystick
3. Ajustar DEADZONE e SENSITIVITY
4. Recalibrar: desconectar e reconectar USB

### Problema: LED não acende

**Verificar:**
1. Polaridade do LED RGB (ânodo/cátodo comum)
2. Resistores 220Ω instalados
3. Conexões PWM (GPIOs 11, 12, 13)
4. Testar com `./pico_mouse_app white`

---

## 📚 Referências

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [TinyUSB Documentation](https://docs.tinyusb.org/)
- [Linux USB Driver Guide](https://www.kernel.org/doc/html/latest/driver-api/usb/)
- [USB HID Specification](https://www.usb.org/hid)

---

## 👥 Autores

**Disciplina:** TPSE2  
**Instituição:** UFC
**Ano:** 2025

---

## 📄 Licença

Este projeto foi desenvolvido para fins educacionais.

---

**✅ Projeto Completo e Funcional - Todos os Requisitos Atendidos!**
