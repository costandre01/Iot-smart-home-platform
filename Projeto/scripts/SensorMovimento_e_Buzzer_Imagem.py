import RPi.GPIO as GPIO
import time
import datetime
import requests
import os
import cv2  # OpenCV para captura de imagem
from bluepy.btle import Scanner, DefaultDelegate

#   instalação do update na BASH
#   sudo apt-get install python3-opencv libatlas-base-dev
#   sudo pip3 install bluepy
#   sudo apt-get install python3-pip libglib2.0-dev
#   pip install bluepy

# Pinos do HC-SR04
TRIG = 23
ECHO = 24

# Pino do buzzer
BUZZER_PIN = 17

# Configuração do GPIO
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)
GPIO.setup(BUZZER_PIN, GPIO.OUT)
pwm_buzzer = GPIO.PWM(BUZZER_PIN, 300)
pwm_buzzer_start = False

webcam_url = os.getenv("IOT_WEBCAM_URL", "http://127.0.0.1:4747/video")

# URL da API
API_URL = os.getenv('IOT_API_URL', 'http://localhost:8080/query/api.php')

UPLOAD_URL = os.getenv('IOT_UPLOAD_URL', 'http://localhost:8080/query/upload.php')

def medir_distancia():
    #Medir a distância
    GPIO.output(TRIG, False)
    time.sleep(0.05)
    GPIO.output(TRIG, True)
    time.sleep(0.00001)
    GPIO.output(TRIG, False)

    pulse_start = time.time()
    timeout = pulse_start + 1
    while GPIO.input(ECHO) == 0:
        pulse_start = time.time()
        if time.time() > timeout:
            return -1

    pulse_end = time.time()
    while GPIO.input(ECHO) == 1:
        pulse_end = time.time()
        if time.time() > timeout:
            return -1

    pulse_duration = pulse_end - pulse_start
    distancia_cm = round(pulse_duration * 17150, 2)
    return distancia_cm

def enviar_para_api(nome,valor):
    #Enviar valor com timestamp via POST
    agora = datetime.datetime.now()
    hora_formatada = agora.strftime("%Y-%m-%d %H:%M:%S")
    payload = {'nome': nome, 'valor': valor, 'hora': hora_formatada}

    try:
        r = requests.post(API_URL, data=payload)
        print(f"[{hora_formatada}] POST enviado: {payload} → Resposta: {r.status_code}")
    except Exception as e:
        print(f"Erro ao enviar para a API: {e}")
def ler_valor_api(nome_sensor):
    # Lêr valor de sensor remoto via GET
    try:
        r = requests.get(f'{API_URL}?nome={nome_sensor}')
        if r.status_code == 200:
            texto = r.text.strip()
            try:
                # Tenta converter para float
                return float(texto)
            except ValueError:
                # Se falhar, retorna como string
                return texto
        else:
            print(f"Erro ao fazer GET {nome_sensor}: {r.status_code}")
            return None
    except Exception as e:
        print(f"Exceção no GET {nome_sensor}: {e}")
        return None
def tirar_foto_e_enviar():
    cap = cv2.VideoCapture(webcam_url)
    ret, frame = cap.read()
    if ret:
        cv2.imwrite('captura.jpg', frame)
        cap.release()
        try:
            files = {'imagem': open('captura.jpg', 'rb')}
            r = requests.post(UPLOAD_URL, files=files)
            if r.status_code == 200:
                print(r.text)
                # Após enviar a foto, atualiza o comando para "Nao"
                enviar_para_api('imagem', 'Nao')
            else:
                print("Erro ao enviar a imagem:", r.status_code)
        except Exception as e:
            print("Erro ao enviar imagem:", e)
    else:
        print("Erro: não foi possível capturar imagem da webcam.")
        cap.release()


# LOOP PRINCIPAL
try:
    while True:

        # Ler comandos remotos
        buzzer_cmd = ler_valor_api('buzzer')
        tirarfoto_cmd = ler_valor_api('imagem') # incompleto

        # Ativar buzzer pelo site
        if buzzer_cmd == "Ligado":
            if not pwm_buzzer_start:  # só liga se estiver desligado
                pwm_buzzer.start(50)
                pwm_buzzer_start = True
                print("Buzzer LIGADO manualmente")
            enviar_para_api('buzzer', 'Ligado')
        elif buzzer_cmd == "Desligado":
            if pwm_buzzer_start:  # só desliga se estiver ligado
                pwm_buzzer.stop()
                pwm_buzzer_start = False
                print("Buzzer DESLIGADO manualmente")
            enviar_para_api('buzzer', 'Desligado')

        # Tirar foto pelo site
        if tirarfoto_cmd == "Sim":
            print("Foto solicitada manualmente")
            tirar_foto_e_enviar()
            enviar_para_api('imagem', 'Nao')

        # Medir e enviar distância
        distancia = medir_distancia()
        if distancia != -1:
            print(f"Distância: {distancia} cm")
            enviar_para_api("movimento", distancia)

        # Ler sensores de ambiente
        temperatura = ler_valor_api('temperatura')
        humidade = ler_valor_api('humidade')
        print(f"Temperatura: {temperatura}°C, Humidade: {humidade}%")


        # Controle automático baseado em temperatura e humidade
        if temperatura > 40 or humidade > 80:
            if not pwm_buzzer_start:
                pwm_buzzer.start(50)
                pwm_buzzer_start = True
                print("Buzzer ATIVADO automaticamente")
                enviar_para_api('buzzer', 'Ligado')

            print("Temperatura ou humidade acima do limite. A tirar foto...")
            tirar_foto_e_enviar()
        else:
            if pwm_buzzer_start:  # só desliga se não foi ligado manualmente
                pwm_buzzer.stop()
                pwm_buzzer_start = False
                print("Buzzer DESLIGADO automaticamente")
                enviar_para_api('buzzer', 'Desligado')

        time.sleep(1)

except KeyboardInterrupt:
    print("\nInterrompido pelo utilizador.")
finally:
    pwm_buzzer.stop()
    GPIO.cleanup()
    print("GPIO limpo. Programa terminado.")
