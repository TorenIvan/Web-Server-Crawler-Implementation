#!/bin/bash -e

echo $0 $1 $2 $3 $4
args=("$@")
echo $@
if [ -f ${args[1]} ]
then
  if [ -s ${args[1]} ]
  then
    echo "Do all the job"
  else
    echo "exit 1"
    exit 1;
  fi
else
  echo "File" ${args[1]} "does not exist, exit 1"
  exit 1;
fi

if [ -d ${args[0]} ]
echo -e "existing Directory::: \c"
then
  if [ $(find ${args[0]} -maxdepth 0 -type d -empty 2>/dev/null) ]
  then
    echo ${args[0]} "and is Empty"
  else
    echo ${args[0]} "and is not Empty"
    rm -rf ${args[0]} && mkdir ${args[0]}
  fi
else
  echo "Directory:::" ${args[0]} "does not exist, exit 1"
  exit 1;
fi

if [ $# != 4 ]
then
  echo "Not right number of args"
  exit 1
else
  echo "Right number of args"
fi

re='^[0-9]+$'
if ! [[ ${args[2]} =~ $re ]]
then
   echo "error: Not a number for w arg"
   exit 1
else
  echo "Right for w arg"
fi
if ! [[ ${args[3]} =~ $re ]]
then
   echo "error: Not a number for p arg"
   exit 1
 else
   echo "Right for p arg"
fi


counter=0
array=()
lines=()
pinakassite=()


echo "arg1 is "${args[1]}


while IFS=:$'\r\n''' read -r line || [[ -n "$line" ]]
do

    savIFS=$IFS
    IFS=\"
    array+=($line)
    IFS=$savIFS

    counter=$((counter+1))

done < "${args[1]}"

if [ $counter -gt 2000 ]
then
  echo "ok for the size of " ${args[1]}
else
  echo ${args[1]} "is too small"
  exit 1
fi

cd ${args[0]}
d="site"
html=".html"
c=0
for ((i=0;i<${args[2]};i++)){
  g=$d$i
  mkdir $g
  r=()
  for((j=0;j<${args[3]};j++)){
    ttemp=$RANDOM
    q=0
    while [[ q < j ]]; do
      if [$ttemp eq ${r[q]}]
      then
        j=$((j-1))
        continue 2
      fi
      done
      r[j]=$ttemp
       temp=$g"/page"$i"_"$((r[j]))$html
       c=$((c+1))
       #echo $temp
       u=i*args[3]+j
       pinakassite[u]=$temp
       #echo "new for pinakassite="${args[0]}"/"${pinakassite[u]}
       touch $temp

  }
}

mmmm=2
mm=${args[3]}
nn=${args[2]}
#echo $mm
let mux=mm*nn
let met=mm/mmmm
let net=nn/2
let met=met+1
let net=net+1
holla=$(((($net)) + (($met))))
for ((i=0;i<${args[2]};i++)){ #sites

  for ((j=0;j<${args[3]};j++)){       #pages
    f=()
    q=()
    for ((o=0;o<met;o++)){   #gia sunolo f
      l=$(($RANDOM % (${args[3]})))   #tuxaia apo 0 eos p

      if  [ "$l" -eq "$j" ]
      then

        o=$(($o-1))
        continue;
      else
        u=i*args[3]+l
        uu=i*args[3]+j
        f1=${pinakassite[u]}
        for ri in "${f[@]}"
		do
    	if [ "$ri" == "$f1" ] ; then
        	o=$(($o-1))

       		continue 2;
       	fi
       	done
       	f[o]=${pinakassite[u]}

      fi
    }
    for ((o=0;o<net;o++)){   #gia sunolo q
      l=$(($RANDOM % ($mux)))   #tuxaia apo 0 eos w
      al=$(((($i))*((${args[3]}))))
      if [ "$l" -ge "$al" ]
      then
        p=$(($al+${args[3]}))
        if [ "$l" -lt "$p" ]
        then
          o=$(($o-1))
          continue ;
        fi
      fi
      u=l
      uu=i*args[3]+j
      q1=${pinakassite[u]}
      for ri in "${q[@]}"
		do
    	if [ "$ri" == "$q1" ] ; then
        	o=$(($o-1))
       		continue 2;
       	fi
       done
       	q[o]=${pinakassite[u]}
    }
    k=$(($RANDOM%($counter-2000)))  #1o bima
    m=$(($(($RANDOM%1001))+1000))  #2o bima
    jjj=20;
    for((b=0;b<$jjj;b++)){
      echo ${pinakassite[b]}
    }
    ekto=$(((($net))+(($met))))
    ex=$((m/ekto))
    ant=$(($ex+$k))

    nm=$((${args[2]}*${args[3]}))
    alll=$(((($i))*((${args[3]}))))
    jm=$(($alll+$met))
    alll=$(($alll+$j))
    jm=$(($jm-1))
    echo "<!DOCTYPE html>" >> ${pinakassite[uu]}
    echo "<html>" >> ${pinakassite[uu]}
    echo "    <body>" >> ${pinakassite[uu]}
    counter2=0
    NEW=("${f[@]}" "${q[@]}")
    arrayy=("${NEW[@]}")
    pp=0;
    for((oo=$k;oo<$(($counter-1));oo+=$(($m/(($net+$met)))))){
          bima=$(($m/(($net+$met))))
          ooo=$oo

      for((llll=$ooo;llll<$(($ooo+$bima));llll++)){
      	echo "    ${array[llll]}" >> ${pinakassite[uu]}
  		}

  		echo "    ${array[llll]} ">> ${pinakassite[uu]}
  		echo "<a href="../../${args[0]}"/"${arrayy[pp]}">link1_text</a>" >> ${pinakassite[uu]}
  		counter2=$(($counter2+1))
  		if [ "$counter2" -eq "$holla" ]
        then
        	break
        fi
        pp=$(($pp+1))
    }
    echo "    </body>" >> ${pinakassite[uu]}
    echo "</html>" >> ${pinakassite[uu]}

  }
}
