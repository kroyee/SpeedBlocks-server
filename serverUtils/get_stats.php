<?php
if ($_SERVER['REMOTE_ADDR'] != '127.0.0.1')
    exit();

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

$sql = "SELECT ";

$postdata = json_decode(file_get_contents('php://input'));
if ($postdata->id == NULL)
    exit();

$fixed = array();
$id = $postdata->id;
unset($postdata->id);

function getTable($table_name) {
    global $id, $fixed, $conn;

    $sql = "SELECT * FROM $table_name WHERE user_id=$id";
    $result = $conn->query($sql);

    if ($result->num_rows == 0) {
        return;
    	//$othersql = "INSERT INTO sb_stats (user_id, avgbpm, gamesplayed, gameswon, heropoints, herorank, maxbpm, maxcombo, points, rank, totalgames, totalbpm, 1vs1points, 1vs1rank, tournamentsplayed, tournamentswon, gradeA, gradeB, gradeC, gradeD) VALUES ($postdata->id, 0, 0, 0, 1500, 0, 0, 0, 1000, 25, 0, 0, 1500, 0, 0, 0, 0, 0, 0, 0)";
    	//$result = $conn->query($othersql);
    	//$result = $conn->query($sql);
    }

    $row = $result->fetch_assoc();

    foreach ($row as $key => $value) {
        if ($value == NULL)
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

foreach ($postdata as $key => $value)
    getTable($key);

echo json_encode($fixed);

$conn->close();
?>