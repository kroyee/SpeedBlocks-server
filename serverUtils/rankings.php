<?php
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

$results_per_page = 5;

if (isset($_GET["page"])) { $page  = $_GET["page"]; } else { $page=1; };
$start_from = ($page-1) * $results_per_page;
$sql = "select username, 1vs1rank, 1vs1points from phpbb_users U inner join sb_stats S on U.user_id=S.user_id order by 1vs1rank asc LIMIT $start_from, ".$results_per_page;
$rs_result = $conn->query($sql);
?>
<table border="1" cellpadding="4">
<tr>
    <td bgcolor="#CCCCCC"><strong>Name</strong></td>
    <td bgcolor="#CCCCCC"><strong>Rank</strong></td><td bgcolor="#CCCCCC"><strong>Points</strong></td></tr>
<?php
 while($row = $rs_result->fetch_assoc()) {
?>
            <tr>
            <td><? echo $row['username']; ?></td>
            <td><? echo $row['1vs1rank']; ?></td>
            <td><? echo $row['1vs1points']; ?></td>
            </tr>
<?php
};
?>
</table>



<?php
$sql = "SELECT COUNT(user_id) AS total FROM sb_stats";
$result = $conn->query($sql);
$row = $result->fetch_assoc();
$total_pages = ceil($row["total"] / $results_per_page); // calculate total pages with results

for ($i=1; $i<=$total_pages; $i++) {  // print links for all pages
            echo "<a href='index.php?page=".$i."'";
            if ($i==$page)  echo " class='curPage'";
            echo ">".$i."</a> ";
};
?>