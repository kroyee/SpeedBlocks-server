<?php
$name = $_POST['name'];
$pass = $_POST['pass'];
$machineid = $_POST['machineid'];
$remember_me = $_POST['remember'];

$servername = "localhost";
$username = "-";
$password = "-";
$dbname = "-";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if (empty($remember_me)) {
	define('IN_PHPBB', true);
	$phpbb_root_path = (defined('PHPBB_ROOT_PATH')) ? PHPBB_ROOT_PATH : '-';
	$phpEx = substr(strrchr(__FILE__, '.'), 1);
	include($phpbb_root_path . 'common.' . $phpEx);

	// Start session management
	$user->session_begin();
	$auth->acl($user->data);
	$user->setup();

	$result = $auth->login($name, $pass); // There are more params but they're optional

	$id = $user->data['user_id'];
	$name = $user->data['username'];
	if (empty($machineid))
	        $machineid = "empty";

	if ($result['status'] == LOGIN_SUCCESS)
	{
        $tmp_hash = bin2hex(random_bytes(10));
        $remember = bin2hex(random_bytes(10));
        $sql = "INSERT INTO sb_auth (user_id, user_name, login_hash, machineid, remember) VALUES ($id, '$name', '$tmp_hash', '$machineid', '$remember') ON DUPLICATE KEY UPDATE user_name='$name', login_hash='$tmp_hash', machineid='$machineid', remember='$remember';";
        if ($conn->query($sql) === TRUE) {
        echo $tmp_hash.$remember;
        } else {
        echo "Error: " . $sql . "<br>" . $conn->error;
        }
	}
	else
	{
	    echo 'Failed';
	}
}
else {
	$sql = "SELECT user_name FROM sb_auth WHERE user_name='$name' AND machineid='$machineid' AND remember='$remember_me'";
	$result = $conn->query($sql);
	if ($result->num_rows == 0) {
		echo 'Failed remember';
	}
	else {
		$tmp_hash = bin2hex(random_bytes(10));
        $remember = bin2hex(random_bytes(10));
        $sql = "UPDATE sb_auth SET login_hash='$tmp_hash', remember='$remember' WHERE user_name='$name' AND machineid='$machineid' AND remember='$remember_me'";
        if ($conn->query($sql) === TRUE) {
        	echo $tmp_hash.$remember;
        } else {
        	echo "Error: " . $sql . "<br>" . $conn->error;
        }
	}
}
?>