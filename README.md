# HomeControl 🏠

Um sistema de automação residencial inteligente baseado em Raspberry Pi Pico W com FreeRTOS

![Status do Projeto](https://img.shields.io/badge/Status-Em%20Desenvolvimento-yellow)
![Licença](https://img.shields.io/badge/Licença-MIT-blue)

## 📋 Sobre o Projeto

O HomeControl é um sistema de automação residencial inteligente que visa melhorar o conforto, a eficiência energética e a segurança doméstica. Utilizando a plataforma Raspberry Pi Pico W com periféricos da BitDogLab/RP2040, o sistema integra sensores e atuadores para monitorar e controlar aspectos como iluminação, temperatura e nível de líquidos.

![Protótipo Inicial]

## ✨ Funcionalidades

### 💡 Controle de Iluminação

- Automação baseada em presença via sensor PIR
- Ajuste de intensidade luminosa via LDR (sensor de luminosidade)
- Controle manual via interface web

### 🌡️ Controle de Temperatura

- Monitoramento contínuo via termistor
- Acionamento automático de ventilador ou aquecedor
- Ajuste manual de temperatura via interface web

### 💧 Monitoramento de Nível de Líquido

- Medição via sensor infravermelho (fototransmissor e fotorreceptor)
- Acionamento automático de bomba ou válvula de água
- Alertas sonoros para níveis críticos
- Visualização e controle via interface web

### 🖥️ Interface Web

- Acesso via Wi-Fi a partir de qualquer dispositivo
- Dashboard com visualização do status de todos os sensores
- Controle manual de todos os atuadores
- Interface intuitiva e responsiva

### 📊 Feedback Visual e Sonoro

- Display OLED 128x64 para informações detalhadas do sistema
- Matriz de LEDs 5x5 para indicação de status
- Buzzer passivo para alertas sonoros

## 🛠️ Tecnologias Utilizadas

- **Hardware**:
  - Raspberry Pi Pico W
  - Periféricos BitDogLab/RP2040
  - Sensores: PIR (presença), LDR (luminosidade), Termistor, Infravermelho
  - Atuadores: Display OLED 128x64, Matriz de LEDs 5x5, Buzzer passivo

- **Software**:
  - FreeRTOS para gerenciamento de tarefas concorrentes
  - lwIP para implementação da pilha TCP/IP
  - Webserver integrado para interface de controle
  - HTML/CSS/JavaScript para interface do usuário

## 🔄 Fluxo de Interação

1. **Inicialização do Sistema**:
   - Sensores começam a monitorar o ambiente
   - Estabelecimento de conexão Wi-Fi
   - Disponibilização da interface web

2. **Ação Automática**:
   - Acionamento da iluminação baseado em presença
   - Ajuste de luminosidade conforme ambiente
   - Controle de temperatura dentro de faixas predefinidas
   - Monitoramento e controle do nível de líquido

3. **Ação Manual**:
   - Controle direto via interface web
   - Ajustes personalizados de iluminação, temperatura e nível de líquido

4. **Feedback ao Usuário**:
   - Informações em tempo real no display OLED
   - Indicações visuais na matriz de LEDs
   - Alertas sonoros para condições críticas

## 📈 Benefícios

- **Conforto**: Ambiente automaticamente ajustado às preferências do usuário
- **Eficiência Energética**: Redução do consumo através de automação inteligente
- **Segurança**: Detecção e alerta sobre condições anormais
- **Conveniência**: Controle remoto via qualquer dispositivo com navegador web

## 🚀 Como Começar

```bash
# Clone este repositório
git clone https://github.com/seu-usuario/home-control.git

# Entre na pasta do projeto
cd home-control

# Siga as instruções de instalação no arquivo INSTALL.md
