<?php
if ($_SERVER['REMOTE_ADDR'] != '127.0.0.1') {
	echo "Error: Could not show credentials";
	exit();
}

$postdata = json_decode(file_get_contents('php://input'));

if ($postdata->challenge_name == NULL) {
	echo "Error: No challenge name specified";
	exit();
}

if ($postdata->id == NULL) {
	echo "Error: No user id specified";
	exit();
}

if ($postdata->time == NULL) {
	echo "Error: No time specified";
	exit();
}

if ($postdata->key != '-') {
	echo "Error: Invalid server key";
	exit();
}

$servername = "localhost";
$username = "-";
$password = "-";
$dbname = "-";

$ch_name = $postdata->challenge_name;
$user_id = $postdata->id;
$time = $postdata->time;
unset($postdata->challenge_name);
unset($postdata->id);
unset($postdata->time);
unset($postdata->key);

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) {
	die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT * FROM information_schema.tables WHERE table_schema = 'SpeedBlocks' AND table_name = '$ch_name';";

$result = $conn->query($sql);

if ($result->num_rows == 0) { // Create new table
	$sql = "create table $ch_name (user_id int(10) PRIMARY KEY, time int(10) NOT NULL, ";
	foreach ($postdata as $key => $value) {
		$sql .= "$key int(10), ";
	}
	$sql = substr($sql, 0, -2);
	$sql .= ");";

	if ($conn->query($sql) === TRUE) {
	    echo "New table created successfully";
	} else {
	    echo "Error: " . $sql . "<br>" . $conn->error;
	    exit();
	}

	$sql = "INSERT INTO challenge_list (name) VALUES ($ch_name);";

	if ($conn->query($sql) === TRUE) {
	    echo "Updated challenge_list succesfully";
	} else {
	    echo "Error: " . $sql . "<br>" . $conn->error;
	    exit();
	}
}

$sql = "INSERT INTO $ch_name (user_id, time, ";

foreach ($postdata as $key => $value)
	$sql .= "$key, ";

$sql = substr($sql, 0, -2) . ") VALUES ($user_id, $time, ";

foreach ($postdata as $key => $value)
	$sql .= "$value, ";

$sql = substr($sql, 0, -2) . ") ON DUPLICATE KEY UPDATE user_id=$user_id, time=$time, ";

foreach ($postdata as $key => $value)
	$sql .= "$key=$value, ";

$sql = substr($sql, 0, -2) . ";";

if ($conn->query($sql) === TRUE) {
    echo "New records created successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>

// Example input
// {"challenge_name":"test","id":412,"time":376,"blocks":12,"highest_combo":57}