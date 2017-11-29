<?php
if ($_SERVER['REMOTE_ADDR'] != '127.0.0.1') {
	echo "Failed";
	exit();
}

$hash = $_POST['hash'];

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

$sql = "SELECT * FROM sb_auth WHERE login_hash='$hash'";

$result = $conn->query($sql);

if ($result->num_rows == 0) {
        echo "Failed";
}
else {
	$row = $result->fetch_assoc();
	$get_time = $row['time'];
	$time_created = strtotime($get_time);
	$time_now = time();
	$id = $row['user_id'];
	$name = $row['user_name'];

	if ($time_now - $time_created < 30) {
		echo $id."%".$name;
	}
	else
		echo "Failed";

	$sql = "UPDATE sb_auth SET login_hash='' WHERE user_id=$id";
	$conn->query($sql);
}

$conn->close();
?>