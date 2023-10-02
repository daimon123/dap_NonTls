var @variable# = {
type: 'doughnut',
data: {
datasets: [{
data: [
@values#
],
backgroundColor: [
//window.chartColors.green,
window.chartColors.yellow,
window.chartColors.red,
window.chartColors.purple,
],
label: 'Level'
}],
labels: [
@level#
]
},
options: {
responsive: true,
legend: {
position: 'top',
},
title: {
display: true,
text: '@title#'
},
animation: {
animateScale: false,
animateRotate: true
}
}
};
