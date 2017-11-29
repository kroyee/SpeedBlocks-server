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
unset($postdata->key);

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

$sql = "DELETE FROM usersonline;";
if ($conn->query($sql) === TRUE) {
    echo "Users online cleared successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$sql = "INSERT INTO usersonline (user_id, mode) VALUES ";
foreach ($postdata as $key => $value)
    $sql .= "($key, $value), ";

$sql = substr($sql, 0, -2) . ";";

if ($conn->query($sql) === TRUE) {
    echo "Users online updated successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>