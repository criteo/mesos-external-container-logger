node {
  stage 'Checkout'
  checkout scm

  stage 'Build Mesos'
  sh "./build-mesos.sh"

  stage 'Compile'
  sh "./compile.sh"
}
