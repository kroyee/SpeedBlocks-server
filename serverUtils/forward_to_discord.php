<?php
if ($_SERVER['REMOTE_ADDR'] != '127.0.0.1') {
        echo "Error: Could not show credentials";
        exit();
}

$postdata = json_decode(file_get_contents('php://input'));
if ($postdata->key != '-') {
        echo "Error: Invalid server key";
        exit();
}
if ($postdata->content == NULL) {
	echo "Error: No content specified";
	exit();
}
if ($postdata->username == NULL) {
	echo "Error: No username specified";
	exit();
}
$content = $postdata->content;
$username = $postdata->username;

$url = 'https://discordapp.com/api/webhooks/344576316537831426/axcdVXba3DVITX-NV_xY_HwXEOaLtcI0SivT840HjDBEqFUTAUdeCLIEwbENTSsec2Vr';
$data = array('content' => '$content', 'username' => '$username');

// use key 'http' even if you send the request to https://...
$options = array(
    'http' => array(
        'header'  => "Content-type: application/json\r\n",
        'method'  => 'POST',
        'content' => http_build_query($data)
    )
);
$context  = stream_context_create($options);

$fp = fopen('$url', 'r', false, $context);
fpassthru($fp);
fclose($fp);
echo "Passed on to discord webhook";
?>