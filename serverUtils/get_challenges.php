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

	$sql = "SELECT * FROM $table_name;";
	$result = $conn->query($sql);

	foreach ($row as $key => $value) {
        if ($value == NULL || $key == "user_id")
            continue;
        $info = $result->fetch_field();
        if (in_array($info->type, array(
        MYSQLI_TYPE_TINY, MYSQLI_TYPE_SHORT, MYSQLI_TYPE_INT24,
        MYSQLI_TYPE_LONG, MYSQLI_TYPE_LONGLONG,
        MYSQLI_TYPE_DECIMAL,
        MYSQLI_TYPE_FLOAT, MYSQLI_TYPE_DOUBLE
        ))) {
                $fixed[$table_name.$key] = 0 + $value;
        } else {
                $fixed[$table_name.$key] = $value;
        }
    }
}

$sql = "SELECT * FROM challenge_list;";

$result = $conn->query($sql);

while (true) {
	$row = $result->fetch_assoc();
	if ($row == NULL)
		break;
	$table_name = $row['name'];
	$sql = "SELECT * FROM $table_name;";

}

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