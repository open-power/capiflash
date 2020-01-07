pipeline {
    parameters {
        choice(name: 'PLATFORM_FILTER', choices: ['all', 'RHEL', 'Ubuntu', 'AIX'], description: 'Run on specific platform')
    }
    agent none
    stages {
        stage('BuildAndTest') {
            matrix {
                agent {
                    label "${PLATFORM}-agent"
                }
                when { anyOf {
                    expression { params.PLATFORM_FILTER == 'all' }
                    expression { params.PLATFORM_FILTER == env.PLATFORM }
                } }
                axes {
                    axis {
                        name 'PLATFORM'
                        values 'RHEL', 'Ubuntu', 'AIX'
                    }
                }
                stages {
                    stage('Build') {
                        steps {
                            sh 'src/build/tools/Jenkins_stage_build'
                        }
                    }
                    stage('Test') {
                        steps {
                            sh 'src/build/tools/Jenkins_stage_test'
                        }
                    }
                }
            }
        }
    }
}
