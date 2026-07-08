<?php
// TasiaGuard self-service unban page
// Drop this on any PHP server and configure the settings below.

// --- CONFIG ---
$API_URL       = 'https://i.let-us.cyou/tasiaguard';
$API_KEY       = 'c4F15DakZxpvHY2G98n2DtNQAtIUkCiM';
$WEBHOOK_URL   = 'https://discord.com/api/webhooks/1524115779799351338/k4V2kJuVFrgBFILv0XFddKS2XTfWptVURpgT96QKNV4hL9dStEPmYMlmqw8tAWPLP4eu'; // Set to a URL to receive POST notifications on unban

$message = '';
$status  = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $ip        = trim($_POST['ip'] ?? '');
    $uuid      = trim($_POST['uuid'] ?? '');
    $first_name = trim($_POST['first_name'] ?? '');
    $last_name  = trim($_POST['last_name'] ?? '');
    $auth_name  = trim($_POST['auth_name'] ?? ''); // must be "Tasia"

    // 1. Validate IP
    if (!filter_var($ip, FILTER_VALIDATE_IP) && !preg_match('/^[0-9.\/]+$/', $ip)) {
        $message = 'Please enter a valid IP address.';
        $status  = 'error';
    }
    // 2. Validate credentials against API
    elseif (empty($uuid) || empty($auth_name)) {
        $message = 'UUID and auth name are required.';
        $status  = 'error';
    } else {
        $ch = curl_init();
        curl_setopt_array($ch, [
            CURLOPT_URL            => $API_URL . '/api/verify-unban',
            CURLOPT_POST           => true,
            CURLOPT_POSTFIELDS     => http_build_query(['uuid' => $uuid, 'name' => $auth_name]),
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_TIMEOUT        => 10,
            CURLOPT_SSL_VERIFYPEER => false,
        ]);
        $verify_raw  = curl_exec($ch);
        $verify_http = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);

        $verify = json_decode($verify_raw, true);

        if ($verify_http !== 200 || !($verify['verified'] ?? false)) {
            $message = 'Authentication failed. Check UUID and name.';
            $status  = 'error';
        } else {
            // 3. Call unban API
            $ch = curl_init();
            curl_setopt_array($ch, [
                CURLOPT_URL            => $API_URL . '/api/unban?key=' . urlencode($API_KEY),
                CURLOPT_POST           => true,
                CURLOPT_POSTFIELDS     => 'ip=' . urlencode($ip),
                CURLOPT_RETURNTRANSFER => true,
                CURLOPT_TIMEOUT        => 15,
                CURLOPT_SSL_VERIFYPEER => false,
            ]);
            $unban_raw  = curl_exec($ch);
            $unban_http = curl_getinfo($ch, CURLINFO_HTTP_CODE);
            $unban_err  = curl_error($ch);
            curl_close($ch);

            if ($unban_err) {
                $message = 'Connection error: ' . htmlspecialchars($unban_err);
                $status  = 'error';
            } elseif ($unban_http === 200) {
                $data    = json_decode($unban_raw, true);
                $message = htmlspecialchars($data['message'] ?? 'IP unbanned successfully.');
                $status  = 'ok';

                // 4. Send Discord webhook notification
                if ($WEBHOOK_URL) {
                    $webhook_data = [
                        'username' => 'TasiaGuard',
                        'avatar_url' => '',
                        'embeds' => [[
                            'title'       => 'Unban request',
                            'description' => "IP **{$ip}** has been unbanned and whitelisted.",
                            'color'       => 5763719,
                            'fields' => [
                                ['name' => 'IP', 'value' => $ip, 'inline' => true],
                                ['name' => 'First name', 'value' => $first_name ?: '-', 'inline' => true],
                                ['name' => 'Last name', 'value' => $last_name ?: '-', 'inline' => true],
                                ['name' => 'UUID', 'value' => $uuid, 'inline' => false],
                                ['name' => 'Time', 'value' => gmdate('Y-m-d H:i:s') . ' UTC', 'inline' => true],
                                ['name' => 'Requester IP', 'value' => $_SERVER['REMOTE_ADDR'] ?? 'unknown', 'inline' => true],
                            ],
                            'footer' => ['text' => 'TasiaGuard I-Grid • Self-service unban'],
                            'timestamp' => gmdate('c'),
                        ]],
                    ];
                    $ch = curl_init($WEBHOOK_URL);
                    curl_setopt_array($ch, [
                        CURLOPT_POST           => true,
                        CURLOPT_POSTFIELDS     => json_encode($webhook_data),
                        CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
                        CURLOPT_RETURNTRANSFER => true,
                        CURLOPT_TIMEOUT        => 10,
                        CURLOPT_SSL_VERIFYPEER => false,
                    ]);
                    curl_exec($ch);
                    curl_close($ch);
                }
            } elseif ($unban_http === 401) {
                $message = 'Service configuration error.';
                $status  = 'error';
            } else {
                $data    = json_decode($unban_raw, true);
                $message = htmlspecialchars($data['error'] ?? 'Unknown error.');
                $status  = 'error';
            }
        }
    }
}

$user_ip = $_SERVER['HTTP_X_FORWARDED_FOR'] ?? $_SERVER['REMOTE_ADDR'] ?? '';
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Unban my IP</title>
<style>
* { box-sizing: border-box; }
body {
  margin: 0; min-height: 100vh;
  display: flex; align-items: center; justify-content: center;
  background: radial-gradient(circle at top, #102b55 0%, #040915 45%, #02050c 100%);
  color: #e6fbff; font-family: system-ui, -apple-system, sans-serif;
}
.card {
  background: linear-gradient(135deg, rgba(8,28,48,.88), rgba(5,14,31,.94));
  border: 1px solid rgba(66,234,255,.36); border-radius: 24px;
  padding: 40px; width: min(480px, calc(100% - 32px));
  box-shadow: 0 0 0 1px rgba(255,255,255,.03) inset, 0 20px 70px rgba(0,0,0,.5), 0 0 50px rgba(61,240,255,.15);
  text-align: center;
}
h1 { margin: 0 0 6px; font-size: 28px; text-shadow: 0 0 22px rgba(61,240,255,.35); }
p { color: #94b9c6; margin: 0 0 24px; font-size: 14px; }
input {
  width: 100%; border-radius: 14px; border: 1px solid rgba(61,240,255,.32);
  padding: 14px 16px; background: rgba(0,0,0,.42); color: #e6fbff;
  font-family: monospace; font-size: 16px; text-align: center; outline: none;
  margin-bottom: 12px;
}
input:focus { border-color: #b7ff3d; box-shadow: 0 0 0 3px rgba(183,255,61,.13); }
button {
  width: 100%; border: 1px solid rgba(61,240,255,.48); border-radius: 999px;
  padding: 14px; color: #fff; background: linear-gradient(90deg, rgba(61,240,255,.32), rgba(183,255,61,.2));
  cursor: pointer; font-weight: 800; font-size: 16px; transition: all .3s;
}
button:hover { box-shadow: 0 0 24px rgba(183,255,61,.3); }
.msg { padding: 14px; border-radius: 14px; margin-bottom: 18px; font-weight: 700; }
.msg.ok { border: 1px solid rgba(106,255,179,.5); background: rgba(106,255,179,.12); color: #6affb3; }
.msg.error { border: 1px solid rgba(255,88,113,.55); background: rgba(255,88,113,.12); color: #ff5871; }
.label { color: #94b9c6; font-size: 12px; text-transform: uppercase; letter-spacing: .1em; margin-bottom: 4px; text-align: left; }
.hint { font-size: 12px; color: #94b9c6; margin-top: 10px; }
</style>
</head>
<body>
<div class="card">
  <h1>Unban my IP</h1>
  <p>Enter your IP and authenticate to request removal from the firewall.</p>

  <?php if ($message): ?>
    <div class="msg <?= $status ?>"><?= $message ?></div>
  <?php endif; ?>

  <form method="post" style="text-align:left;">
    <div class="label">Your IP address</div>
    <input name="ip" placeholder="1.2.3.4" value="<?= htmlspecialchars($user_ip) ?>" required>

    <div class="label">First name</div>
    <input name="first_name" placeholder="John" required>

    <div class="label">Last name</div>
    <input name="last_name" placeholder="Doe" required>

    <div class="label">User UUID</div>
    <input name="uuid" placeholder="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" required>

    <div class="label">Name of Marty's daughter (auth)</div>
    <input name="auth_name" placeholder="" required>

    <button type="submit">Unban me</button>
  </form>
  <div class="hint">Your IP: <?= htmlspecialchars($user_ip) ?: 'unknown' ?></div>
</div>
</body>
</html>
