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

$servername = "localhost";
$username = "-";
$password = "-";
$dbname = "-";

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) {
	die("Connection failed: " . $conn->connect_error);
}

$fixed = array();

function getChallengeScores($table_name) {
	global $conn, $fixed;

	$sql = "SELECT username as name, a.* FROM $table_name a inner join phpbb_users b on a.user_id=b.user_id;";
	$result = $conn->query($sql);

	if ($result->num_rows == 0)
		return;

	$fixed[$table_name] = array();

	while ($row = $result->fetch_assoc()) {
		$field_count = 0;
		$user_id = $row['user_id'];
		$fixed[$table_name][$user_id] = array();
		foreach ($row as $key => $value) {
	        if ($value == NULL || $key == "user_id")
	            continue;
	        $info = $result->fetch_field_direct($field_count++);
	        if (in_array($info->type, array(
	        MYSQLI_TYPE_TINY, MYSQLI_TYPE_SHORT, MYSQLI_TYPE_INT24,
	        MYSQLI_TYPE_LONG, MYSQLI_TYPE_LONGLONG,
	        MYSQLI_TYPE_DECIMAL,
	        MYSQLI_TYPE_FLOAT, MYSQLI_TYPE_DOUBLE
	        ))) {
	                $fixed[$table_name][$user_id][$key] = 0 + $value;
	        } else {
	                $fixed[$table_name][$user_id][$key] = $value;
	        }
	    }
	}
}

$sql = "SELECT * FROM challenge_list;";

$result = $conn->query($sql);

while ($row = $result->fetch_assoc()) {
	getChallengeScores($row['name']);
}

echo json_encode($fixed);

$conn->close();
?>