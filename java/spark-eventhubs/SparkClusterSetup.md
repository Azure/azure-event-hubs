# How to Setup a Spark Cluster in Azure

This guide explains how a spark cluster was setup on Azure VMs running Ubuntu Server 14.04. It's here so people spend less time configuring or more time building - if anything is wrong/needs updating, please contribute to this page! Thanks :)

This guide assumes you have an Azure subscription that's able to spin up resources. Also, I'm providing what worked for me...almost everything here can be changed/tweaked somehow. If you're looking for more, Apache Spark's website has great documentation that goes much more in depth. 

### 1. Create your VMs
Info: https://azure.microsoft.com/en-us/documentation/articles/virtual-machines-windows-hero-tutorial/

Create as many VMs as you need for your cluster - once you make it through this guide once, tweaking your cluster shouldn't be too difficult. 

### 2. Install Apache Hadoop
I installed Hadoop using the following commands. Get the correct download link at [Apache Hadoop's Release page](http://hadoop.apache.org/releases.html). Pay attention to your version number, because you'll need it when you install Apache Spark. 
```
wget http://mirrors.sonic.net/apache/hadoop/common/CORRECT_VERSION.tar.gz
sudo tar zxvf ~/hadoop-2.7.2.tar.gz -C /usr/local
sudo mv /usr/local/hadoop-* /usr/local/Hadoop
sudo chown –R your_username /usr/local/spark
```

If this method of installation isn't ideal for whatever reason, check out Apache Hadoop's [setup page](http://hadoop.apache.org/docs/current/hadoop-project-dist/hadoop-common/SingleCluster.html) might help.

### 3. Install Apache Spark
On Ubuntu, I installed Spark like so:
```
wget http://www-us.apache.org/dist/spark/CORRECT_VERSION.tgz -P ~/Downloads
sudo tar zxvf ~/Downloads/spark-1.6.2-bin-hadoop2.6.tgz -C /usr/local
sudo mv /usr/local/spark-* /usr/local/spark
sudo chown –R your_username /usr/local/spark
```

Please visit the [Apache Spark Downloads](http://spark.apache.org/downloads.html) page to get the correct link. And, of course, feel free to use a different path if you'd like. Check out Apache Spark's [overview page](http://spark.apache.org/docs/latest/) for more info.

### 4. Install Java
```
sudo add-apt-repository ppa:webupd8team/java
sudo apt-get update
sudo apt-get install oracle-java8-installer
java --version  # to make sure you have Java 8!
```

### 5. Install Scala
```
sudo apt-get update
sudo apt-get install scala
scala –version  # to make sure you have the desired version of Scala
```

### 6. Set JAVA_HOME and SPARK_HOME environment variables 
Add the following to ~./profile. 
```
export JAVA_HOME=/usr/lib/jvm/java-8-oracle      # Make sure this is still the correct path
export PATH=$PATH:$JAVA_HOME/bin

export SPARK_HOME=/usr/local/spark               # Update the path if you used a different location
export PATH=$PATH:$SPARK_HOME/bin
```

Then run the following command to load the environment variables:
```
. ~/.profile
```

### 7. Create spark-env.sh
Go to *$SPARK_HOME/conf/* and create a file called *spark-env.sh*. Place the following in the file for your **Worker** nodes:
```
export SPARK_PUBLIC_DNS='NODE_PUBLIC_IP'
export SPARK_LOCAL_IP='NODE_PRIVATE_IP'
```

Place the following in the *spark-env.sh* for your **Master** nodes:
```
export SPARK_MASTER_IP='NODE_PRIVATE_IP'
export SPARK_LOCAL_IP='NODE_PUBLIC_IP'
```

The public and private IPs can be found in the Azure portal. 

### 8. Create log4j.properties 
In *$SPARK_HOME/conf* you can also make a file called *log4j.properties* to change how Spark logs information. I thought this site was pretty helpful:
https://www.mkyong.com/logging/log4j-log4j-properties-examples/

### 9. Setup slaves file
On your Master node, you should create a file in *$SPARK_HOME/conf* called *slaves* - place the following in the file:
```
your_user_name@WORKER1_PUBLIC_IP
your_user_name@WORKER2_PUBLIC_IP
your_user_name@WORKER3_PUBLIC_IP
...
...
your_user_name@WORKERN_PUBLIC_IP
```

After that you can run *$SPARK_HOME/sbin/start-all.sh* to start up the cluster and *$SPARK_HOME/sbin/stop-all.sh* to stop the cluster. Go to *MASTER_PUBLIC_IP:8080* - the Spark UI should show up and you'll be able to see how many registered workers you have. Hope this helps! Have fun. 
