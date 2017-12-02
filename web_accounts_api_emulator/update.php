<?php
$servername = "localhost";
$username = "electrod_admin";
$password = "admin";
$dbname = "electrod_encash";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
} 
$newAmount  = $_GET['amount'];
$id = $_GET['id'];
$sql = "
          UPDATE `accounts`
          SET `amount` = '".$newAmount."'
          WHERE `id` = '". $id ."';
    ";

$result = $conn->query($sql);

if ($result->num_rows > 0) {
    // output data of each row
    while($row = $result->fetch_assoc()) {
     
    }
} else {
    echo "0 results";
}
$conn->close();
?>