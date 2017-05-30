#!/usr/bin/env bash
#
# start worker and client services
#

usage="Usage: ustore-run.sh [ arguments ]\n
        --config <configuration file> : if the configuration file is not in the default location (conf/config)\n"

# get environment variables
. `dirname "${BASH_SOURCE-$0}"`/ustore_env.sh

# create log directory
if [ ! -d $USTORE_LOG ]; then
  mkdir $USTORE_LOG
fi

# go to ustore home to execute binary
cd $USTORE_HOME

# ssh and start ustore processes
ssh_options="-oStrictHostKeyChecking=no \
             -oUserKnownHostsFile=/dev/null \
             -oLogLevel=quiet"
ustore_run="bin/ustored $@"
http_run="bin/ustore_http $@"
ustore_sshrun="cd $USTORE_HOME; $ustore_run"

# start all the workers
host_file=$USTORE_CONF/workers
# old_IFS=$IFS
# IFS=$'\n'
echo "------------- Starting workers -------------"
for i in `cat $host_file` ; do
  host=`echo $i | cut -d ':' -f 1`
  port=`echo $i | cut -d ':' -f 2`
  if [ $host = localhost ] ; then
    echo Starting worker @ $host : $ustore_run --node_id $i
    $ustore_run --node_id $i >> $USTORE_LOG/worker-$host-$port.log 2>&1 &
    sleep 1
  else
    echo Starting worker @ $host : $ustore_sshrun --worker --node_id $i
    ssh $ssh_options $host $ustore_sshrun --node_id $i >> $USTORE_LOG/worker-$host-$port.log 2>&1 &
    sleep 1
  fi
done

# start the http client
sleep 2
echo Starting http client service @ `hostname`: $http_run
cd $USTORE_HOME
$http_run >> $USTORE_LOG/ustore_http.log 2>&1 &
sleep 1
echo "----------- All processes started ------------"

# IFS=$old_IFS
