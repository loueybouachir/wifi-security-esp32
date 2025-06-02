import requests
from bs4 import BeautifulSoup
from collections import defaultdict, deque
import statistics
import time
from datetime import datetime

# Configuration
ESP32_URL = "http://192.168.137.15/combined_results"
INTERVAL = 60  # secondes entre chaque analyse
MAX_RETRIES = 2  # Nombre maximum de tentatives de connexion
RETRY_DELAY = 5  # Délai entre les tentatives en secondes
analysis_count = 0
history_buffer = deque(maxlen=3)

def print_status(message, emoji="ℹ️"):
    timestamp = time.strftime("%H:%M:%S", time.localtime())
    print(f"\n{emoji} [{timestamp}] {message}")

def download_html(url):
    retry_count = 0
    last_exception = None
    
    while retry_count <= MAX_RETRIES:
        try:
            print_status(f"Tentative de connexion au serveur {url} (essai {retry_count + 1}/{MAX_RETRIES + 1})...", "🔌")
            response = requests.get(url, timeout=10)
            response.raise_for_status()
            
            with open('debug_original.html', 'w', encoding='utf-8') as f:
                f.write(response.text)
            print_status("Téléchargement réussi", "✅")
            return response.text
            
        except requests.exceptions.RequestException as e:
            last_exception = e
            print_status(f"Erreur lors de la tentative {retry_count + 1}: {e}", "❌")
            retry_count += 1
            if retry_count <= MAX_RETRIES:
                print_status(f"Nouvelle tentative dans {RETRY_DELAY} secondes...", "⏳")
                time.sleep(RETRY_DELAY)
    
    print_status(f"Échec après {MAX_RETRIES + 1} tentatives", "🛑")
    raise last_exception

def parse_html(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    wifi_scan_results, frame_results = [], []
    tables = soup.find_all('table')
    if len(tables) < 2:
        print_status("Moins de 2 tables trouvées", "⚠️")
        return wifi_scan_results, frame_results

    for row in tables[0].find_all('tr')[1:]:
        cols = row.find_all('td')
        if len(cols) == 4:
            try:
                wifi_scan_results.append({
                    'SSID': cols[0].get_text().strip(),
                    'BSSID': cols[1].get_text().strip(),
                    'RSSI': int(cols[2].get_text().strip()),
                    'AuthMode': cols[3].get_text().strip()
                })
            except:
                continue

    for row in tables[1].find_all('tr')[1:]:
        cols = row.find_all('td')
        if len(cols) == 6:
            try:
                frame_results.append({
                    'Channel': int(cols[0].get_text().strip()),
                    'RSSI': int(cols[1].get_text().strip()),
                    'Type': cols[2].get_text().strip(),
                    'SourceMAC': cols[3].get_text().strip(),
                    'SSID': cols[4].get_text().strip(),
                    'Length': int(cols[5].get_text().strip())
                })
            except:
                continue

    print_status(f"Analyse HTML : {len(wifi_scan_results)} APs / {len(frame_results)} trames", "✅")
    return wifi_scan_results, frame_results

def evaluate_auth(auth):
    auth = str(auth).strip()
    if auth in ["WPA3", "WPA3-SAE", "3"]:
        return "Très sécurisé"
    elif auth in ["WPA2", "WPA2-PSK", "4"]:
        return "Correct"
    elif auth in ["WPA", "WEP", "2"]:
        return "Vulnérable"
    elif auth.lower() == "open" or auth == "1":
        return "Non sécurisé"
    return f"Inconnu ({auth})"

def detect_anomalies(scan_data, frame_data):
    ssid_aps = defaultdict(list)
    mesh_candidates = defaultdict(set)

    for ap in scan_data:
        if ap["SSID"] != "No SSID":
            ssid_aps[ap["SSID"]].append((ap["BSSID"], ap.get("RSSI", -100), ap.get("AuthMode", "Unknown")))

    for frame in frame_data:
        if frame["SSID"] != "No SSID":
            mesh_candidates[frame["SSID"]].add(frame["SourceMAC"])

    print("\n📡 Comportement des réseaux similaires :")
    evil_twins = {}
    mesh_networks = {}
    
    for ssid, aps in ssid_aps.items():
        if len(aps) <= 1:
            continue
            
        print(f"\nSSID: {ssid}")
        
        # Évaluer la sécurité de chaque AP
        security_levels = [evaluate_auth(auth) for _, _, auth in aps]
        has_open = any(sec in ["Non sécurisé", "Vulnérable"] for sec in security_levels)
        has_secure = any(sec in ["Correct", "Très sécurisé"] for sec in security_levels)
        
        for i, (bssid, rssi, auth) in enumerate(aps, 1):
            print(f"  AP {i}: BSSID={bssid}, RSSI={rssi} dBm, Sécurité={evaluate_auth(auth)}")
        
        # Détection Evil Twin (même SSID mais sécurité différente)
        if has_open and has_secure:
            evil_twins[ssid] = aps
            print("⚠️  **EVIL TWIN DÉTECTÉ !** - Même SSID avec différents niveaux de sécurité")
        elif len(aps) > 1 and all(sec == security_levels[0] for sec in security_levels):
            mesh_networks[ssid] = aps
            print("✅  Réseau Mesh probable - Même SSID et même sécurité sur tous les APs")
        else:
            print("❓ Réseau avec plusieurs points d'accès")

    # Détection des réseaux Mesh basée sur les trames
    mesh_from_frames = {ssid: list(macs) for ssid, macs in mesh_candidates.items() if len(macs) > 1}
    
    return evil_twins, {**mesh_networks, **mesh_from_frames}

def calculate_rssi_average(scan_data):
    rssi_values = defaultdict(list)
    for ap in scan_data:
        if ap["SSID"] != "No SSID":
            rssi_values[ap["SSID"]].append(ap["RSSI"])
    return {ssid: round(statistics.mean(values), 2) for ssid, values in rssi_values.items()}

def compare_last_analyses(buffer):
    print("\n🔁 Changements RSSI / sécurité :")
    scan1, scan2, scan3 = buffer
    all_ssids = set(ap['SSID'] for ap in scan3)
    old_ssids = set(ap['SSID'] for ap in scan1)

    new_networks = all_ssids - old_ssids
    if new_networks:
        print("\n🆕 Réseaux apparus :")
        for ssid in new_networks:
            print(f"  ➕ {ssid}")

    disappeared = old_ssids - all_ssids
    if disappeared:
        print("\n❌ Réseaux disparus :")
        for ssid in disappeared:
            print(f"  ➖ {ssid}")

    for ap_now in scan3:
        ssid, bssid = ap_now["SSID"], ap_now["BSSID"]
        rssi_now, auth_now = ap_now["RSSI"], ap_now["AuthMode"]
        for ap_before in scan1:
            if ap_before["SSID"] == ssid and ap_before["BSSID"] == bssid:
                rssi_diff = rssi_now - ap_before["RSSI"]
                if abs(rssi_diff) >= 10:
                    print(f"  📶 {ssid} ({bssid}) : RSSI {ap_before['RSSI']} → {rssi_now} dBm ({'↗️' if rssi_diff > 0 else '↘️'})")
                if auth_now != ap_before["AuthMode"]:
                    print(f"  🔒 {ssid} ({bssid}) : AuthMode {ap_before['AuthMode']} → {auth_now}")

def average_rssi_over_3_analyses(buffer):
    print("\n📊 Moyenne RSSI sur 3 analyses :")
    rssi_accumulator = defaultdict(list)
    for analysis in buffer:
        for ap in analysis:
            key = (ap['SSID'], ap['BSSID'])
            rssi_accumulator[key].append(ap['RSSI'])

    for (ssid, bssid), values in rssi_accumulator.items():
        if len(values) == 3:
            avg = round(statistics.mean(values), 2)
            print(f"  SSID: {ssid}, BSSID: {bssid} → RSSI moyen: {avg} dBm")

def main():
    global analysis_count
    analysis_count += 1
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"\n🆔 Analyse #{analysis_count} | Date & Heure : {timestamp}")
    print_status("Démarrage de l'analyse Wi-Fi", "🚀")

    try:
        html = download_html(ESP32_URL)
        scan_data, frame_data = parse_html(html)

        if not scan_data:
            print_status("Aucune donnée valide", "🛑")
            return

        print_status("Évaluation de la sécurité", "🔒")
        for ap in scan_data:
            sec = evaluate_auth(ap["AuthMode"])
            print(f"  SSID: {ap['SSID']}, BSSID: {ap['BSSID']} → {sec}")

        evil_twins, mesh_networks = detect_anomalies(scan_data, frame_data)

        print("\n⚠️ Réseaux suspects :")
        for ssid, bssids in evil_twins.items():
            print(f"  SSID: {ssid} → BSSIDs multiples: {bssids}")

        print("\n🔗 Réseaux Mesh détectés :")
        for ssid, macs in mesh_networks.items():
            print(f"  SSID: {ssid} → Nœuds: {macs}")

        avg_rssi = calculate_rssi_average(scan_data)
        print("\n📊 RSSI moyen (analyse courante) :")
        for ssid, rssi in avg_rssi.items():
            print(f"  SSID: {ssid} → {rssi} dBm")

        history_buffer.append(scan_data)

    except Exception as e:
        print_status(f"Erreur : {e}", "❌")
    finally:
        print_status(f"Fin de l'analyse #{analysis_count}", "🛑")

def run_analysis_with_ai(interval_sec=60):
    analysis_cycle = 0  # Nouveau compteur pour suivre le nombre d'analyses
    
    while True:
        main()
        analysis_cycle += 1

        # Ne faire l'analyse intelligente que tous les 3 cycles complets
        if analysis_cycle % 3 == 0:
            if len(history_buffer) == 3:
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                print("\n" + "="*60)
                print(f"🧠 ANALYSE COMPARATIVE INTELLIGENTE (Cycle {analysis_cycle}) | Date & Heure : {timestamp}")
                print("="*60)
                compare_last_analyses(history_buffer)
                average_rssi_over_3_analyses(history_buffer)
                print("="*60 + "\n")
            else:
                print_status("Pas assez de données pour l'analyse intelligente (3 analyses requises)", "⚠️")

        time.sleep(interval_sec)

if __name__ == "__main__":
    run_analysis_with_ai(INTERVAL)