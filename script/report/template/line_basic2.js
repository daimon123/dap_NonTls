var @variable# = {
type: 'line',
data: {
labels: [@category#],
datasets: [{
label: @label2#,
backgroundColor: window.chartColors.yellow,
borderColor: window.chartColors.yellow,
data: [
@warning#
],
fill: false,
}, {
label: @label3#,
backgroundColor: window.chartColors.red,
borderColor: window.chartColors.red,
data: [
@critical#
],
fill: false,
}, {
label: @label4#,
backgroundColor: window.chartColors.purple,
borderColor: window.chartColors.purple,
data: [
@block#
],
fill: false,
}]
},
options: {
responsive: true,
title: {
display: true,
text: '@title#'
},
tooltips: {
mode: 'index',
intersect: false,
},
hover: {
mode: 'nearest',
intersect: true
},
scales: {
xAxes: [{
display: true,
scaleLabel: {
display: true,
labelString: 'Time'
}
}],
yAxes: [{
display: true,
scaleLabel: {
display: true,
labelString: 'Value'
}
}]
}
}
};
