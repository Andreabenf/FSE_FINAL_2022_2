# FSE_FINAL_2022_2

Repositório para documentação do trabalho final da disciplina de Fundamentos de Sistemas Embarcados.

## Descrição

O objetivo deste trabalho é criar sensores e atuadores distribuídos baseados nos microcontroladores ESP32 interconectados via Wifi através do protocolo MQTT, podendo ser aplicada em diversos contextos de automação a exemplo das áreas de Automação Residencial, Predial ou Industrial.
Os microcontroladores ESP32 irão controlar a aquisição de dados de sensores, botões e chaves e acionar saídas como leds, dentre outras. Haverão dois modos de operação dos dispositivos: modo energia que representa o dispositivo ligado à tomada e modo bateria que representa o dispositivo que deverá operar em modo de baixo consumo.

## Integrantes

**Nome** | **Matricula** | **GitHub** 
---------|:-------------:|:----------:
André Aben-Athar de Freitas | 170056155 | Andreabenf
Lucas Ganda | 170039668 | lucasgandac
Wictor Bastos Girardi | 170047326 | wictorGirardi   

## Sensores Utilizados
- Sensor de temperatura
- LED 7 Cores
- Buzzer
- Sensor de som 
- Sensor de efeito hall
- Sensor de reed switch

## Execução
Para execução do projeto para seguir esses passos :

- Clone o projeto
```
git clone https://github.com/Andreabenf/FSE_2022_2_TRAB2
```

- Entre na pasta raiz do projeto
```
cd FSE_2022_2_TRAB2
```
- Exporte, configure e builde a biblioteca idf
```
. ../esp-idf/export.sh
idf.py menuconfig
idf.py build fullclean
```

- Para transpor o código para placa execute 
```
idf.py -p /dev/tty.usbserial-0001 flash monitor
```
obs : Nesse passo o nome do dispositivo após o /dev/ pode variar

### Modo Low Power

- Para acionamento do modo de baixo consumo de energia, é necessário entrar na config e ativar o modo

# Requisitos :

| Requisito | Feito ? | Como fazer |
| -------- | -------- | -------- |
| 
Dashboard criado representando a telemetria e estados dos atributos.    |  Sim    | Feito   |
| Mecanismo RPC para acionamento remoto de atributos dos dispositivos distribuídos. | Sim | Feito|
| Correto acionamento e envio do estado da saída de acordo com os comandos RPC do Dashboard (incluíndo o acionamento em modo PWM). | Sim | Feito|
|  Correta detecção e envio do estado da entrada ao servidor central. | Sim | Feito |
| Correta operação da placa em modo low power economizando energia. | Sim | Inicialização com a flag específica |
| Correta detecção e envio do estado da entrada ao servidor central sendo acordado somente no acionamento da GPIO em modo low power. | Sim | Pull-up do botão |
| Armazenamento do estado dos atributos na NVS. | Sim | Feito |
| Correta implementação de comunicação entre os dispositivos (MQTT e Json). | Sim | Feito |
| Implementação de mecanismo de fácil acesso para que o usuário possa fazer cadastrar as credenciais do Wifi na ESP32. | Sim |Feito |