
# FSE_FINAL_2022_2

Repositório para documentação do trabalho final da disciplina de Fundamentos de Sistemas Embarcados.

## Descrição

O objetivo deste trabalho é criar sensores e atuadores distribuídos baseados nos microcontroladores ESP32 interconectados via Wifi através do protocolo MQTT, podendo ser aplicada em diversos contextos de automação a exemplo das áreas de Automação Residencial, Predial ou Industrial.
Os microcontroladores ESP32 irão controlar a aquisição de dados de sensores, botões e chaves e acionar saídas como leds, dentre outras. Haverão dois modos de operação dos dispositivos: modo energia que representa o dispositivo ligado à tomada e modo bateria que representa o dispositivo que deverá operar em modo de baixo consumo.

## Vídeo de apresentação 
https://youtu.be/frOsKpH8AAY

## Link Dashboard
http://164.41.98.25:443/dashboards/63287f50-a42f-11ed-8436-09bf763c9306
<img width="1437" alt="Captura de Tela 2023-02-14 às 21 31 10" src="https://user-images.githubusercontent.com/40725728/218894466-5c482d96-838c-4abd-8a2d-023449691c41.png">


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
- Realize o download do IDF e coloque ele na raiz do seu projeto
link esp-idf: https://drive.google.com/drive/folders/1sSgixs9aZkyl3wJ67GTF0wxB3VakxNuD?usp=sharing

- Entre na pasta raiz do projeto
```
cd FSE_2022_2_TRAB2/client
```
- Exporte, configure e builde a biblioteca idf
```

. ../esp-idf/export.sh
idf.py menuconfig
idf.py build fullclean
```

No menu, entre nas configuraçes de Serial flasher

![image](https://user-images.githubusercontent.com/32902438/219026849-05e4bd2c-260b-42b7-812f-a35a2b803da7.png)

e mude o flash size para 4mb:

![image](https://user-images.githubusercontent.com/32902438/219026946-19ae9653-eefe-4502-b4a0-d80813caad68.png)

No partition table, retire o 's' de partititons e deixe partition.csv

![image](https://user-images.githubusercontent.com/32902438/219027008-5e0f028b-9977-4a0f-a93d-e20192bf0737.png)


Agora, configure sua placa e wifi:
![image](https://user-images.githubusercontent.com/32902438/219027085-f4c41e59-eed4-4c1a-ac0e-b2e75edb8316.png)
<img width="1059" alt="Captura de Tela 2023-02-14 às 21 22 10" src="https://user-images.githubusercontent.com/40725728/218893551-88d48583-d55f-45be-bb45-334fcc555681.png">
<img width="1059" alt="Captura de Tela 2023-02-14 às 21 22 16" src="https://user-images.githubusercontent.com/40725728/218893559-d5a5051b-15c9-49fb-97f7-cf739bfa610e.png">
<img width="1059" alt="Captura de Tela 2023-02-14 às 21 22 23" src="https://user-images.githubusercontent.com/40725728/218893566-77e47c24-9e5e-414a-8190-aee0b1a0d2b0.png">

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
| Dashboard criado representando a telemetria e estados dos atributos.    |  Sim    | Feito   |
| Mecanismo RPC para acionamento remoto de atributos dos dispositivos distribuídos. | Sim | Feito|
| Correto acionamento e envio do estado da saída de acordo com os comandos RPC do Dashboard (incluíndo o acionamento em modo PWM). | Sim | Feito|
|  Correta detecção e envio do estado da entrada ao servidor central. | Sim | Feito |
| Correta operação da placa em modo low power economizando energia. | Sim | Inicialização com a flag específica |
| Correta detecção e envio do estado da entrada ao servidor central sendo acordado somente no acionamento da GPIO em modo low power. | Sim | Pull-up do botão |
| Armazenamento do estado dos atributos na NVS. | Sim | Feito |
| Correta implementação de comunicação entre os dispositivos (MQTT e Json). | Sim | Feito |
| Implementação de mecanismo de fácil acesso para que o usuário possa fazer cadastrar as credenciais do Wifi na ESP32. | Sim |Feito |
