<?php
if ($_SERVER['REMOTE_ADDR'] != '127.0.0.1') {
    echo "Error: Could not show credentials";
    exit();
}

$postdata = json_decode(file_get_contents('php://input'));

if ($postdata->user_id == NULL) {
    echo "Error: No user_id specified";
    exit();
}

if ($postdata->table_name == NULL) {
    echo "Error: No table_name specified";
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

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$table_name = $postdata->table_name;

unset($postdata->table_name);
unset($postdata->key);

function columnExists($column_name) {
    global $conn, $postdata, $table_name;
    $sql = "SELECT table_name FROM information_schema.columns WHERE column_name='$column_name' and table_name='$table_name'";
    $result = $conn->query($sql);
    if ($result->num_rows == 0) {
        $sql = "ALTER TABLE $table_name ADD $column_name int(8);";
        $conn->query($sql);
    }
}

$sql = "INSERT INTO $table_name (";

foreach ($postdata as $key => $value)
    columnExists($key);

foreach ($postdata as $key => $value)
    $sql .= "$key, ";

$sql = substr($sql, 0, -2) . ") VALUES (";

foreach ($postdata as $key => $value)
    $sql .= "$value, ";

$sql = substr($sql, 0, -2) . ") ON DUPLICATE KEY UPDATE ";

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